'''
Implements following processes
1) baseline quantization that trains both the output range and weight
2) batch normalization folding to the preceding layer (we currently support only CONV-BN-ACT folding)
3) quantization aware retraining (clamp output range according to the ADC spec)
4) final inferencing (hardware aware)
'''
from models import *

import torch
import torchvision
import torchvision.models as models
import torchvision.datasets as datasets


import os
import argparse

import qa_conv

import config as cfg
import constant as const
import dataloader
from dataloader import scratch_train
from dataloader import test, retrain, reset, train, valid
import torch.cuda.amp

from qa_conv.hardware import PIMUnit

# Set cudnn benchmark to True (to enable cudnn autotuner)
torch.backends.cudnn.benchmark=True
torch.set_printoptions(threshold=1_000)

# Disable 
# 1) error detection
# 2) profiling
# options for faster training
torch.autograd.set_detect_anomaly(False)
torch.autograd.profiler.profile(False)
torch.autograd.profiler.emit_nvtx(False)



# Initialize the network according to the configuration
print('==> Building model..', flush=True)
const.init_network()

# Initialize the data loader
print('==> Preparing data..', flush=True)
dataloader.init_loader()



# set the previous lsit and last layer
if cfg.MULTI_GPU:
    prev_list = const.qnet.module.prev_list
else:
    prev_list = const.qnet.prev_list
if cfg.MULTI_GPU:
    const.last_layer=[".module" + layer for layer in const.last_layer]

# Load the pretrained data to the quantized network
## CONFIG: bitwidths for (weight, bias, bn_weight, bn_bias, activation)
# we assume QLC cell and use even and odd row for signed 5-bit
# for bias, we use the whole array; therefore, we can use 128 rows to represent a single bias
# 128 rows => 2**7 => 7 additional bits for bias
qa_conv.config.config_network(qa_conv.config.Config(5, 12, 5))
qa_conv.config.set_last(const.qnet, '', layer_list=const.last_layer)

cfg.best_acc = 0

if cfg.MODE == cfg.MODE_TYPE.BASELINEQUANT:
    if "ResNet" in cfg.NETWORK:
        # We heuristically set the log2 for ResNet-type networks
        if cfg.MULTI_GPU:
            const.qnet.module.load_state_dict(const.onet.state_dict(), strict=False)
        else:
            const.qnet.load_state_dict(const.onet.state_dict(), strict=False)
        qa_conv.config.init_log2_network(const.qnet, acti=2.5849625, weight=1.5849625, name='')
    elif "VGG" in cfg.NETWORK:
        # We profile the distribution and set the log2 for VGG-type networks

        # profile the network using CPU (qnet_profile)
        const.qnet_profile.load_state_dict(const.onet.state_dict(), strict=False)
        # add hook => for profiling
        qa_conv.threshold.add_hook(const.qnet_profile,
                               '',
                               qa_conv.threshold.hook_handler,
                               end_list=(
                                   nn.Conv2d, nn.Linear, nn.ReLU,
                                   qa_conv.fold.ShareQuant
                               ), show=True)
        # set to the floating point phase when profiling the distribution
        qa_conv.config.set_quant_phase(const.qnet_profile, quant_phase=PIMUnit.QuantPhase.FLOAT)
        test(const.qnet_profile, cali=True)

        # profile the maximum distribution (quantization range is set according to the maximum distribution)
        qa_conv.threshold.init_network(const.qnet_profile, '', weight_method='max', \
                                   acti_method='max', show=True)

        if cfg.MULTI_GPU:
            const.qnet.module.load_state_dict(const.qnet_profile.state_dict(), strict=False)
        else:
            const.qnet.load_state_dict(const.qnet_profile.state_dict(), strict=False)
    else:
        assert(0 and "Should implement custom optimizer and scheduler for other type of networks")

    del const.onet
    del const.qnet_profile
    epoch_start = -1
    cfg.best_acc = 0

    if "ResNet" in cfg.NETWORK:
        epoch_end = 110
    elif "VGG" in cfg.NETWORK:
        epoch_end = 10
        # just a heuristic => we should normalize the weight for the VGG-like networks
        qa_conv.config.set_normalize(const.qnet, mode=False)

    const.qnet = const.qnet.to(cfg.device)

    quant_p = [p for n, p in const.qnet.named_parameters() if n.find('log2') != -1]
    normal_p = [p for n, p in const.qnet.named_parameters() if n.find('log2') == -1]

    # set the optimizer and scheduler
    optimizer2 = None
    if "ResNet" in cfg.NETWORK:
        optimizer1 = torch.optim.SGD(const.qnet.parameters(), lr=0.01 * (cfg.RETRAIN_BATCH / 1024.), momentum=0.9, weight_decay=1e-4)
        scheduler1 = torch.optim.lr_scheduler.StepLR(optimizer1, step_size=30, gamma=0.1)
        optimizer2 = None
        scheduler2 = None
    elif "VGG" in cfg.NETWORK:
        optimizer1 = torch.optim.Adam(quant_p, lr=1e-2 * (cfg.RETRAIN_BATCH / 128.), betas=(0.9, 0.999))
        scheduler1 = torch.optim.lr_scheduler.StepLR(optimizer1, step_size=1, gamma=0.5)
        optimizer2 = torch.optim.Adam(normal_p, lr=1e-6 * (cfg.RETRAIN_BATCH / 128.), betas=(0.9, 0.999))
        scheduler2 = torch.optim.lr_scheduler.StepLR(optimizer2, step_size=1, gamma=0.5)
    else:
        assert(0 and "Should implement custom optimizer and scheduler for other type of networks")


    qa_conv.fold.fold_the_network(const.qnet, prev_list = prev_list)
    qa_conv.config.set_quant_phase(const.qnet, quant_phase=PIMUnit.QuantPhase.BASELINEQUANT)
    qa_conv.config.make_bn_freeze(const.qnet, mode=False, convert=False, show=False)

    scaler = torch.cuda.amp.GradScaler()
    for epoch in range(epoch_start + 1, epoch_end):
        scratch_train(const.qnet, epoch, scaler, optimizer1, scheduler1, optimizer2, scheduler2)
        valid(const.qnet, epoch, optimizer1, scheduler1, optimizer2, scheduler2, scaler, quantize=True, inference=False)
else:
    del const.onet
    del const.qnet_profile

    # generate next list from the prev_list
    qa_conv.fold.fold_the_network(const.qnet, prev_list=prev_list)

    if cfg.MODE == cfg.MODE_TYPE.FOLDINGBN:
        checkpoint = torch.load(cfg.CHECKPOINT_PATH+'/scratch_qckpt_' + const.model_name_q + '.pth', map_location=cfg.device)
    elif cfg.MODE == cfg.MODE_TYPE.QACONV:
        checkpoint = torch.load(cfg.CHECKPOINT_PATH+'/scratch_no_bn_qckpt_' + const.model_name_q + '.pth', map_location=cfg.device)
    elif cfg.MODE == cfg.MODE_TYPE.INFERENCE:
        checkpoint = torch.load(cfg.CHECKPOINT_PATH+'/scratch_no_bn_clamp_qckpt_' + const.model_name_q + '.pth', map_location=cfg.device)
    #checkpoint = torch.load(cfg.CHECKPOINT_PATH+'/scratch_no_bn_qckpt_' + const.model_name_q + '.pth', map_location=cfg.device)

    if cfg.MULTI_GPU:
        const.qnet.module.load_state_dict(checkpoint['net'], strict=True)
    else:
        const.qnet.load_state_dict(checkpoint['net'], strict=True)

    #const.qnet = const.qnet.to(cfg.device)
    #if cfg.MULTI_GPU:
    #    const.qnet = torch.nn.DataParallel(const.qnet, device_ids=cfg.DEVICE_IDS)
    print(checkpoint['acc'])
    
    qa_conv.config.set_last(const.qnet, '', layer_list=const.last_layer)

    if cfg.MODE == cfg.MODE_TYPE.FOLDINGBN:
        qa_conv.config.set_quant_phase(const.qnet, quant_phase=PIMUnit.QuantPhase.FOLDINGBN)
    elif cfg.MODE == cfg.MODE_TYPE.QACONV:
        qa_conv.config.set_quant_phase(const.qnet, quant_phase=PIMUnit.QuantPhase.QACONV)
    elif cfg.MODE == cfg.MODE_TYPE.INFERENCE:
        qa_conv.config.set_quant_phase(const.qnet, quant_phase=PIMUnit.QuantPhase.INFERENCE)
    else:
        assert(0 and "Undefined retraining phase")
    #qa_conv.config.set_quant_phase(const.qnet, quant_phase=PIMUnit.QuantPhase.FOLDINGBN)

    if cfg.MODE == cfg.MODE_TYPE.FOLDINGBN:
        qa_conv.config.make_bn_freeze(const.qnet, mode=True, show=False, convert=True)
    else:
        qa_conv.config.make_bn_freeze(const.qnet, mode=True, show=False, convert=False)

    if "VGG" in cfg.NETWORK:
        qa_conv.config.set_normalize(const.qnet, mode=False)

    if cfg.MODE == cfg.MODE_TYPE.QACONV or cfg.MODE == cfg.MODE_TYPE.INFERENCE:
        qa_conv.config.set_qa_conv_spec(const.qnet, precision = 10, min_range_log2 = -1, max_range_log2 = 9, prev_list = prev_list)
        qa_conv.config.print_log2_t_for_pim(const.qnet, prev_list = prev_list) 

    if cfg.MODE == cfg.MODE_TYPE.INFERENCE:
        test(const.qnet)
    else:

        quant_p_acti = [p for n, p in const.qnet.named_parameters() if (n.find('log2') != -1 and n.find('acti_log2') != -1)]
        quant_p_weight = [p for n, p in const.qnet.named_parameters() if (n.find('log2') != -1 and n.find('acti_log2') == -1)]
        quant_p_bias = [p for n, p in const.qnet.named_parameters() if (n.find('log2') != -1 and n.find('bias_log2') != -1)]

        normal_p = [p for n, p in const.qnet.named_parameters() if n.find('log2') == -1]

        if cfg.MODE == cfg.MODE_TYPE.QACONV:
            optimizer1, scheduler1, optimizer2, scheduler2, optimizer3, scheduler3 = \
                reset(quant_p_acti, quant_p_weight, quant_p_bias, normal_p, reduce_step=True)
        else:
            optimizer1, scheduler1, optimizer2, scheduler2, optimizer3, scheduler3 = \
                reset(quant_p_acti, quant_p_weight, quant_p_bias, normal_p, reduce_step=False)
        

        cfg.best_acc = 0  # reset best test accuracy
        epoch_end = 20
        scaler = torch.cuda.amp.GradScaler()
        for epoch in range(0, epoch_end):
            retrain(const.qnet, epoch, optimizer1, optimizer2, optimizer3, scheduler1, scheduler2, scheduler3, scaler)
            if optimizer1 is not None:
                print(optimizer1.param_groups[0]['lr'], end=" ")
            if optimizer2 is not None:
                print(optimizer2.param_groups[0]['lr'], end = " ")
            if optimizer3 is not None:
                print(optimizer3.param_groups[0]['lr'], end = " ")
            print()
