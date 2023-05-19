import torch
import torch.nn as nn
import torch.nn.functional as F
from .foldmodule import _FoldModule
import numpy as np

from ..hardware import bn_norm
from ..hardware import PIMUnit


class Conv2dBN(_FoldModule):
    # weight / bias / activation

    def __init__(self, conv, bn):
        super().__init__()
        if isinstance(conv, nn.Conv2d) and isinstance(bn, nn.BatchNorm2d):
            self.layer_data_type = {"weight": True, "bias": True, "acti": False}
            self.conv = conv
            self.bn = bn
            
            self.pim_unit = conv.pim_unit
        else:
            raise Exception('The folded function does not meet type check')

        self.bn_frozen = False
        self.normalize = True

        self.weight_log2_t = self.conv.weight_log2_t
        self.bias_log2_t = self.conv.bias_log2_t

    def set_pim_unit(self, pim_unit):
        self.pim_unit = pim_unit
        self.conv.set_pim_unit(pim_unit)

    def set_fix_log2(self):
        if self.weight_log2_t != None:
            self.weight_log2_t.requires_grad_(False)
        if self.bias_log2_t != None:
            self.bias_log2_t.requires_grad_(False)

    def set_train_log2(self):
        if self.weight_log2_t != None:
            self.weight_log2_t.requires_grad_(True)
        if self.bias_log2_t != None:
            self.bias_log2_t.requires_grad_(True)


    def bn_freeze(self, mode=True, convert=True):
        self.bn_frozen = mode
        if mode and convert:
            bn_var = self.bn.running_var.detach().clone().data.reshape(-1, 1, 1, 1)
            bn_mean = self.bn.running_mean.detach().clone().data.reshape(-1, 1, 1, 1)
            bn_weight = self.bn.weight.detach().clone().data.reshape(-1, 1, 1, 1)
            bn_bias = self.bn.bias.detach().clone().data.reshape(-1, 1, 1, 1)
            if self.conv.bias is not None:
                conv_bias = self.conv.bias.reshape(-1, 1, 1, 1)
            else:
                conv_bias = 0.

            conv_weight = self.conv.weight * bn_weight / (bn_var + self.bn.eps).sqrt()
            weight_scale = torch.max(conv_weight) / torch.max(self.conv.weight)
            conv_bias = bn_weight * (conv_bias - bn_mean) / (
                bn_var + self.bn.eps).sqrt() + bn_bias
            self.weight_log2_t = torch.nn.Parameter(self.conv.weight_log2_t + weight_scale.log2())
            self.bias_log2_t = torch.nn.Parameter(torch.stack([conv_bias.abs().max().detach().data.log2()]))

        if mode:
            self.pim_unit.weight_log2_t = self.weight_log2_t
            self.pim_unit.bias_log2_t = self.bias_log2_t
        else:
            self.pim_unit.weight_log2_t = self.conv.weight_log2_t
            self.pim_unit.bias_log2_t = self.conv.bias_log2_t

    def forward(self, input):
        bn_var = self.bn.running_var.reshape(-1, 1, 1, 1)
        bn_mean = self.bn.running_mean.reshape(-1, 1, 1, 1)
        bn_weight = self.bn.weight.reshape(-1, 1, 1, 1)
        bn_bias = self.bn.bias.reshape(-1, 1, 1, 1)
        if self.conv.bias is not None:
            conv_bias = self.conv.bias.reshape(-1, 1, 1, 1)
        else:
            conv_bias = torch.zeros(1).to(self.conv.weight.device)

        if self.normalize:
            mean = self.conv.weight.mean()
            std = self.conv.weight.std()
            weight = self.conv.weight.add(-mean).div(std)
        else:
            weight = self.conv.weight

        if not PIMUnit.quant_phase == PIMUnit.QuantPhase.FLOAT \
           and self.bn_frozen:

            conv_weight, conv_bias = bn_norm(weight, conv_bias, bn_weight, bn_var, bn_mean, bn_bias, self.bn.eps)

            conv_weight = self.pim_unit.quant_weight(conv_weight)
            conv_bias = self.pim_unit.quant_bias(conv_bias)
            conv_bias = conv_bias.reshape(-1)


            if PIMUnit.quant_phase != PIMUnit.QuantPhase.INFERENCE:
                inter = F.conv2d(input,
                                             conv_weight,
                                             conv_bias.reshape(-1),
                                             self.conv.stride, self.conv.padding,
                                             self.conv.dilation, self.conv.groups)
            else:
                inter = []
                spatial_iter_granularity = int(PIMUnit.num_rows / (conv_weight.shape[2] * conv_weight.shape[2]))

                in_channel = 0
                while in_channel < input.shape[1]:
                    if in_channel + spatial_iter_granularity >= input.shape[1]: spatial_iter_granularity = (input.shape[1] - in_channel)
                    if in_channel == 0:
                        inter.append(F.conv2d(input[:, in_channel:in_channel+spatial_iter_granularity, :, :],
                                              conv_weight[:, in_channel:in_channel+spatial_iter_granularity, :, :],
                                              conv_bias.reshape(-1),
                                              self.conv.stride, self.conv.padding,
                                              self.conv.dilation, self.conv.groups))
                    else:
                        inter.append(F.conv2d(input[:, in_channel:in_channel+spatial_iter_granularity, :, :],
                                                            conv_weight[:, in_channel:in_channel+spatial_iter_granularity, :, :],
                                                            None,
                                                            self.conv.stride, self.conv.padding,
                                                            self.conv.dilation, self.conv.groups))
                    in_channel += max_row

        else:
            inter = self.conv(input)
            inter = self.bn(inter)

        return inter
