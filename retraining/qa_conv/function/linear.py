"""
Provide quantilized form of torch.nn.modules.linear
"""

import torch
import torch.nn as nn
import torch.nn.functional as F
import math

import numpy as np

from ..hardware import PIMUnit


class Linear(nn.Linear):
    def __init__(self,
                 in_features,
                 out_features,
                 bias=True):
        super().__init__(in_features, out_features, bias=bias)

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


    def linear_forward(self, input):
        weight = self.pim_unit.quant_weight(self.weight)
        if self.bias is not None:
            bias = self.pim_unit.quant_bias(self.bias)
        else:
            bias = self.bias

        if PIMUnit.quant_phase != PIMUnit.QuantPhase.INFERENCE:
            inter = F.linear(input, weight, bias)
        else:
            inter = []
            spatial_iter_granularity = PIMUnit.num_rows

            while in_channel < input.shape[1]:
                in_channel = 0
                while in_channel < input.shape[1]:
                    if in_channel + spatial_iter_granularity >= input.shape[1]: spatial_iter_granularity = (input.shape[1] - in_channel)
                    if in_channel == 0:
                        inter.append(F.linear(input[:, in_channel:in_channel+spatial_iter_granularity],
                                                            weight[:, in_channel:in_channel+spatial_iter_granularity],
                                                            bias.reshape(-1)))
                    else:
                        inter.append(F.linear(input[:, in_channel:in_channel+spatial_iter_granularity],
                                                            weight[:, in_channel:in_channel+spatial_iter_granularity],
                                                            None))
                    in_channel += spatial_iter_granularity

        return inter

    def forward(self, input):
        if self.last or PIMUnit.quant_phase == PIMUnit.QuantPhase.FLOAT:
            return F.linear(input, self.weight, self.bias)
        else:
            return self.linear_forward(input)
