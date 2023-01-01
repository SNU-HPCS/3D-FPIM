import torchvision
import torchvision.datasets as datasets
import torchvision.transforms as transforms
import torch.optim as optim
import torch.nn as nn
import numpy as np

import qa_conv
import os

from utils import progress_bar
import torch

import constant as const
import config as cfg

import torch.cuda.amp
from pathlib import Path

criterion = nn.CrossEntropyLoss()

# data loaders
retrainloader=None
validloader=None
testloader=None
calibloader=None

def init_loader():
    global validloader
    global testloader
    global retrainloader
    global calibloader
    # SET imagenet path

    imagenet_data = '/home/luke7515/Lab_Project/FlashPIM/ILSVRC_ImageNet'
    traindir = os.path.join(imagenet_data, 'train')
    valdir = os.path.join(imagenet_data, 'val')
    testdir = os.path.join(imagenet_data, 'test')
    normalize = transforms.Normalize(mean=[0.485, 0.456, 0.406],
                                     std=[0.229, 0.224, 0.225])
    
    # Set three different loaders
    train_dataset = datasets.ImageFolder(
        traindir,
        transforms.Compose([
            transforms.RandomResizedCrop(const.IMAGE_SIZE),
            transforms.RandomHorizontalFlip(),
            transforms.ToTensor(),
            normalize,
        ]))
    
    seed=3
    torch.backends.cudnn.deterministic = True
    np.random.seed(seed)
    torch.manual_seed(seed)
    torch.cuda.manual_seed(seed)

    retrainloader = torch.utils.data.DataLoader(
        train_dataset, batch_size=cfg.RETRAIN_BATCH, shuffle=True,
        num_workers=24, pin_memory=True, worker_init_fn=np.random.seed(seed))
    
    validloader = torch.utils.data.DataLoader(
        datasets.ImageFolder(valdir, transforms.Compose([
            transforms.Resize(256),
            transforms.CenterCrop(const.IMAGE_SIZE),
            transforms.ToTensor(),
            normalize,
        ])),
        batch_size=cfg.TEST_BATCH, shuffle=True,
        num_workers=24, pin_memory=True, worker_init_fn=np.random.seed(seed))
    
    testloader = torch.utils.data.DataLoader(
        datasets.ImageFolder(valdir, transforms.Compose([
            transforms.Resize(256),
            transforms.CenterCrop(const.IMAGE_SIZE),
            transforms.ToTensor(),
            normalize,
        ])),
        batch_size=cfg.TEST_BATCH, shuffle=True,
        num_workers=12, pin_memory=True, worker_init_fn=np.random.seed(seed))
    
    calibloader = torch.utils.data.DataLoader(
        datasets.ImageFolder(valdir, transforms.Compose([
            transforms.Resize(256),
            transforms.CenterCrop(const.IMAGE_SIZE),
            transforms.ToTensor(),
            normalize,
        ])),
        batch_size=cfg.CALIB_BATCH, shuffle=True,
        num_workers=12, pin_memory=True, worker_init_fn=np.random.seed(seed))

def train(net, epoch, optimizer):
    print('\nEpoch: %d' % epoch)
    net.train()
    train_loss = 0
    correct = 0
    total = 0
    for batch_idx, (inputs, targets) in enumerate(retrainloader):
        inputs, targets = \
            inputs.to(cfg.device, non_blocking=True), \
            targets.to(cfg.device, non_blocking=True)
        optimizer.zero_grad()
        outputs = net(inputs)
        loss = criterion(outputs, targets)
        loss.backward()
        optimizer.step()

        train_loss += loss.item()
        _, predicted = outputs.max(1)
        total += targets.size(0)
        correct += predicted.eq(targets).sum().item()

        progress_bar(batch_idx, len(retrainloader), 'Loss: %.3f | Acc: %.3f%% (%d/%d)'
                     % (train_loss/(batch_idx+1), 100.*correct/total, correct, total))

def scratch_train(net, epoch, scaler, optimizer1, scheduler1, optimizer2, scheduler2):
    print('\nEpoch: %d' % epoch)
    if optimizer1 != None and hasattr(optimizer1, 'param_groups'):
        print("optimizer1", optimizer1.param_groups[0]['lr'])
    if optimizer2 != None and hasattr(optimizer2, 'param_groups'):
        print("optimizer2", optimizer2.param_groups[0]['lr'])

    net.train()
    train_loss = 0
    correct = 0
    total = 0
    for batch_idx, (inputs, targets) in enumerate(retrainloader):
        inputs, targets = \
            inputs.to(cfg.device, non_blocking=True), \
            targets.to(cfg.device, non_blocking=True)
        optimizer1.zero_grad(set_to_none=True)
        if optimizer2 != None:
            optimizer2.zero_grad(set_to_none=True)

        with torch.cuda.amp.autocast():
            outputs = net(inputs)
            loss = criterion(outputs, targets)

        scaler.scale(loss).backward()

        if optimizer1 != None:
            scaler.step(optimizer1)
        if optimizer2 != None:
            scaler.step(optimizer2)
        scaler.update()

        train_loss += loss.item()
        _, predicted = outputs.max(1)
        total += targets.size(0)
        correct += predicted.eq(targets).sum().item()

        progress_bar(batch_idx, len(retrainloader), 'Loss: %.3f | Acc: %.3f%% (%d/%d)'
                     % (train_loss/(batch_idx+1), 100.*correct/total, correct, total))

    if scheduler1 != None:
        scheduler1.step()
    if scheduler2 != None:
        scheduler2.step()


def retrain(net, epoch, optimizer1, optimizer2, optimizer3, scheduler1, scheduler2, scheduler3, scaler, freeze=True):
    global testloader
    global validloader
    global retrainloader
    global calibloader
    print('\nEpoch: %d' % epoch)
    net.train()
    train_loss = 0
    correct = 0
    total = 0
    for batch_idx, (inputs, targets) in enumerate(retrainloader):
        inputs, targets = \
            inputs.to(cfg.device, non_blocking=True), \
            targets.to(cfg.device, non_blocking=True)
        if not optimizer1 == None:
            optimizer1.zero_grad()
        if not optimizer2 == None:
            optimizer2.zero_grad()
        if not optimizer3 == None:
            optimizer3.zero_grad()
        outputs = net(inputs)
        loss = criterion(outputs, targets)

        scaler.scale(loss).backward()
        #loss.backward()
        if not optimizer1 == None:
            scaler.step(optimizer1)
            #optimizer1.step()
        if not optimizer2 == None:
            scaler.step(optimizer2)
            #optimizer2.step()
        if not optimizer3 == None:
            scaler.step(optimizer3)
            #optimizer3.step()
        if not scheduler1 == None:
            scheduler1.step()
        if not scheduler2 == None:
            scheduler2.step()
        if not scheduler3 == None:
            scheduler3.step()
        scaler.update()

        train_loss += loss.item()
        _, predicted = outputs.max(1)
        total += targets.size(0)
        correct += predicted.eq(targets).sum().item()

        progress_bar(batch_idx, len(retrainloader), 'Loss: %.3f | Acc: %.3f%% (%d/%d)'
                     % (train_loss/(batch_idx+1), 100.*correct/total, correct, total))

        # test every 0.1 epoch for check
        if (batch_idx + 1) % len(retrainloader) == 0:
            if hasattr(optimizer1, 'param_groups') and hasattr(optimizer2, 'param_groups'):
                print(optimizer1.param_groups[0]['lr'], optimizer2.param_groups[0]['lr'], optimizer3.param_groups[0]['lr'])
            if freeze == True:
                valid(net, epoch, optimizer1, scheduler1, optimizer2, scheduler2, scaler, quantize=True, inference=False)
                if hasattr(net, "prev_list"):
                    if cfg.MULTI_GPU:
                        prev_list = net.module.prev_list
                    else:
                        prev_list = net.prev_list
                    qa_conv.config.log2_t_for_pim(net, prev_list)
            else:
                valid(net, epoch, optimizer1, scheduler1, optimizer2, scheduler2, scaler, quantize=True, inference=False)

def valid(net, epoch, optimizer1, scheduler1, optimizer2, scheduler2, scaler, inference=False, quantize=False, cali=False):
    global testloader
    global validloader
    global retrainloader
    global calibloader
    net.eval()
    test_loss = 0
    correct = 0
    correct5 = 0
    total = 0
    with torch.no_grad():
        if cali:
            for batch_idx, (inputs, targets) in enumerate(calibloader):
                inputs, targets = inputs.to(cfg.device), targets.to(cfg.device)
                outputs = net(inputs)
                return
        else:
            for batch_idx, (inputs, targets) in enumerate(validloader):
                inputs, targets = \
                    inputs.to(cfg.device, non_blocking=True), \
                    targets.to(cfg.device, non_blocking=True)
                outputs = net(inputs)
                loss = criterion(outputs, targets)
                test_loss += loss.item()
                _, predicted = outputs.max(1)
                _, predicted5 = outputs.topk(max((1,5)), 1, True, True)
                total += targets.size(0)
                correct += predicted.eq(targets).sum().item()
                targets_resize = targets.view(-1,1)
                correct5 += torch.eq(predicted5, targets_resize).sum().float().item()
                progress_bar(batch_idx, len(validloader), 'Loss: %.3f | Acc: %.3f%% (%d/%d) | Acc5: %.3f%% (%d/%d)'
                             % (test_loss/(batch_idx+1), 100.*correct/total, correct, total, 100*correct5/total, correct5, total))

    # Save checkpoint.
    acc = 100.*correct/total
    if not inference:
        print('Saving..')
        if cfg.MULTI_GPU:
            state = {
                'net': net.module.state_dict(),
                'acc': acc,
                'epoch': epoch
            }
        else:
            state = {
                'net': net.state_dict(),
                'acc': acc,
                'epoch': epoch
            }

        if optimizer1 != None:
            state["optimizer1"] = optimizer1.state_dict()
            state["scheduler1"] = scheduler1.state_dict()
        if optimizer2 != None:
            state["optimizer2"] = optimizer2.state_dict()
            state["scheduler2"] = scheduler2.state_dict()


        Path(cfg.CHECKPOINT_PATH + '/' + cfg.NETWORK).mkdir(parents=True, exist_ok=True)
        #if not os.path.isdir('./checkpoint/'):
        #    os.mkdir('./checkpoint/')
        #if not os.path.isdir('./checkpoint/' + cfg.DIRNAME):
        #    os.mkdir('./checkpoint/' + cfg.DIRNAME)
        #if not os.path.isdir('./checkpoint/' + cfg.DIRNAME + '/' + cfg.NETWORK):
        #    os.mkdir('./checkpoint/' + cfg.DIRNAME + '/' + cfg.NETWORK)

        if cfg.MODE == cfg.MODE_TYPE.BASELINEQUANT:
            if acc > cfg.best_acc:
                torch.save(state, cfg.CHECKPOINT_PATH + '/' + cfg.NETWORK + '/scratch_qckpt_' + const.model_name_q + '_best.pth')
            torch.save(state, cfg.CHECKPOINT_PATH + '/scratch_qckpt_' + const.model_name_q + '.pth')
            torch.save(state, cfg.CHECKPOINT_PATH + '/' + cfg.NETWORK + '/scratch_qckpt_' + const.model_name_q + '_' + str(epoch) + '.pth')
        
        if cfg.MODE == cfg.MODE_TYPE.FOLDINGBN:
            if acc > cfg.best_acc:
                torch.save(state, cfg.CHECKPOINT_PATH + '/' + cfg.NETWORK + '/scratch_no_bn_qckpt_' + const.model_name_q + '_best.pth')
            torch.save(state, cfg.CHECKPOINT_PATH + '/scratch_no_bn_qckpt_' + const.model_name_q + '.pth')
            torch.save(state, cfg.CHECKPOINT_PATH + '/' + cfg.NETWORK + '/scratch_no_bn_qckpt_' + const.model_name_q + '_' + str(epoch) + '.pth')
        
        if cfg.MODE == cfg.MODE_TYPE.QACONV:
            if acc > cfg.best_acc:
                torch.save(state, cfg.CHECKPOINT_PATH + '/' + cfg.NETWORK + '/scratch_no_bn_clamp_qckpt_' + const.model_name_q + '_best.pth')
            torch.save(state, cfg.CHECKPOINT_PATH + '/scratch_no_bn_clamp_qckpt_' + const.model_name_q + '.pth')
            torch.save(state, cfg.CHECKPOINT_PATH + '/' + cfg.NETWORK + '/scratch_no_bn_clamp_qckpt_' + const.model_name_q + '_' + str(epoch) + '.pth')

        if acc > cfg.best_acc:
            cfg.best_acc = acc

# for calibration or just testing purpose
def test(net, cali=False):
    global testloader
    global validloader
    global retrainloader
    global calibloader
    net.eval()
    test_loss = 0
    correct = 0
    correct5 = 0
    total = 0
    with torch.no_grad():
        if cali:
            for batch_idx, (inputs, targets) in enumerate(calibloader):
                #inputs, targets = inputs.to(cfg.device), targets.to(cfg.device)
                inputs, targets = inputs.to('cpu'), targets.to('cpu')
                outputs = net(inputs)
                return
        else:
            for batch_idx, (inputs, targets) in enumerate(testloader):
                inputs, targets = \
                    inputs.to(cfg.device, non_blocking=True), \
                    targets.to(cfg.device, non_blocking=True)
                outputs = net(inputs)
                loss = criterion(outputs, targets)
                test_loss += loss.item()
                _, predicted = outputs.max(1)
                _, predicted5 = outputs.topk(max((1,5)), 1, True, True)
                total += targets.size(0)
                correct += predicted.eq(targets).sum().item()
                targets_resize = targets.view(-1,1)
                correct5 += torch.eq(predicted5, targets_resize).sum().float().item()
                progress_bar(batch_idx, len(testloader), 'Loss: %.3f | Acc: %.3f%% (%d/%d) | Acc5: %.3f%% (%d/%d)'
                             % (test_loss/(batch_idx+1), 100.*correct/total, correct, total, 100*correct5/total, correct5, total))


def reset(quant_p_acti, quant_p_weight, quant_p_bias, normal_p, reduce_step=False):
    if not len(quant_p_acti) == 0:
        if not reduce_step:
            optimizer1 = torch.optim.Adam(quant_p_acti, lr=1e-2, betas=(0.9, 0.999))
        else:
            #optimizer1 = torch.optim.Adam(quant_p_acti, lr=0, betas=(0.9, 0.999))
            optimizer1 = None
    else: 
        optimizer1 = None

    if not len(quant_p_weight) == 0:
        if not reduce_step:
            optimizer2 = torch.optim.Adam(quant_p_weight, lr=1e-2, betas=(0.9, 0.999))
        else:
            optimizer2 = None
            #optimizer2 = torch.optim.Adam(quant_p_weight, lr=0, betas=(0.9, 0.999))
    else: optimizer2 = None
    
    if not len(normal_p) == 0:
        if cfg.NETWORK == "ResNet18" or cfg.NETWORK == "ResNet50":
            optimizer3 = torch.optim.Adam(normal_p, lr=1e-5, betas=(0.9, 0.999))
        if cfg.NETWORK == "VGG16" or cfg.NETWORK == "VGG19":
            optimizer3 = torch.optim.Adam(normal_p, lr=1e-5, betas=(0.9, 0.999))
    else: optimizer3 = None
    
    scale = float(128 / cfg.RETRAIN_BATCH)

    if not len(quant_p_acti) == 0:
        if not reduce_step:
            scheduler1 = torch.optim.lr_scheduler.StepLR(optimizer1, \
                             step_size=int(10000 * scale), gamma=0.8)
            scheduler2 = torch.optim.lr_scheduler.StepLR(optimizer2, \
                             step_size=int(10000 * scale), gamma=0.8)
        else:
            scheduler1 = None
            scheduler2 = None
            #scheduler2 = torch.optim.lr_scheduler.StepLR(optimizer2, \
            #                 step_size=int(10000 * scale), gamma=0.5)
    else: 
        scheduler1 = None 
        scheduler2 = None
    
    
    if not len(normal_p) == 0:
        if not reduce_step:
            scheduler3 = torch.optim.lr_scheduler.StepLR(optimizer3, \
                         step_size=int(10000 * scale), gamma=0.8)
        else:
            scheduler3 = torch.optim.lr_scheduler.StepLR(optimizer3, \
                         step_size=int(10000 * scale), gamma=0.8)
    else: scheduler3 = None

    return optimizer1, scheduler1, optimizer2, scheduler2, optimizer3, scheduler3
