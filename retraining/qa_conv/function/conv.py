"""
Provide quantilized form of torch.nn.modules.conv
"""

import torch
import torch.nn as nn
import torch.nn.functional as F
import math

import numpy as np

from ..hardware import PIMUnit


class Conv2d(nn.Conv2d):
    def __init__(self,
                 in_channels,
                 out_channels,
                 kernel_size,
                 stride=1,
                 padding=0,
                 dilation=1,
                 groups=1,
                 bias=True,
                 padding_mode='zeros'):
        super().__init__(in_channels=in_channels,
                         out_channels=out_channels,
                         kernel_size=kernel_size,
                         stride=stride,
                         padding=padding,
                         dilation=dilation,
                         groups=groups,
                         bias=bias,
                         padding_mode=padding_mode)

        self.layer_data_type = {"weight": True, "bias": True, "acti": False}

        # weight / bias / activation
        self.pim_unit = None
        self.last = False
        self.normalize = True

        self.weight_log2_t = nn.Parameter(torch.Tensor(1))
        self.bias_log2_t = nn.Parameter(torch.Tensor(1))

    def set_pim_unit(self, pim_unit):
        self.pim_unit = pim_unit

    def set_fix_log2(self):
        self.weight_log2_t.requires_grad_(False)
        self.bias_log2_t.requires_grad_(False)

    def set_train_log2(self):
        self.weight_log2_t.requires_grad_(True)
        self.bias_log2_t.requires_grad_(True)

    def conv_forward(self, input):
        if self.normalize:
            mean = self.weight.mean()
            std = self.weight.std()
            weight_q = self.weight.add(-mean).div(std)
        else:
            weight_q = self.weight

        
        weight = self.pim_unit.quant_weight(weight_q)

        if self.bias is not None:
            bias = self.pim_unit.quant_bias(self.bias)
        else:
            bias = self.bias

        # partial output from each crossbar array
        if PIMUnit.quant_phase != PIMUnit.QuantPhase.INFERENCE:
            inter = F.conv2d(input, weight, bias, self.stride, self.padding, self.dilation, self.groups)
        else:
            inter = []
            spatial_iter_granularity = int(PIMUnit.num_rows / (weight.shape[2] * weight.shape[2]))

            in_channel = 0
            while in_channel < input.shape[1]:
                if in_channel + spatial_iter_granularity >= input.shape[1]: spatial_iter_granularity = (input.shape[1] - in_channel)
                if in_channel == 0:
                    inter.append(F.conv2d(input[:, in_channel:in_channel+spatial_iter_granularity, :, :],
                                          weight[:, in_channel:in_channel+spatial_iter_granularity, :, :],
                                          bias.reshape(-1),
                                          self.stride, self.padding,
                                          self.dilation, self.groups))
                else:
                    inter.append(F.conv2d(input[:, in_channel:in_channel+spatial_iter_granularity, :, :],
                                          weight[:, in_channel:in_channel+spatial_iter_granularity, :, :],
                                          None,
                                          self.stride, self.padding,
                                          self.dilation, self.groups))
                in_channel += spatial_iter_granularity

        return inter

    def forward(self, input):
        if PIMUnit.quant_phase == PIMUnit.QuantPhase.FLOAT:
            return F.conv2d(input, self.weight, self.bias, self.stride,
                            self.padding, self.dilation, self.groups)
        else: 
            return self.conv_forward(input)
