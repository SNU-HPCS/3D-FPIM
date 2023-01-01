"""
Provide quantilized form of torch.nn.modules.activation
"""

import torch
import torch.nn as nn
import torch.nn.functional as F

from ..hardware import PIMUnit


class ReLU(nn.ReLU):

    def __init__(self,
                 inplace=False):
        super().__init__(inplace)
        # weight / bias / activation
        self.layer_data_type = {"weight": False, "bias": False, "acti": True}
        
        self.pim_unit = None

        self.acti_log2_t = nn.Parameter(torch.Tensor(1))

    def set_pim_unit(self, pim_unit):
        self.pim_unit = pim_unit

    def set_fix_log2(self):
        self.acti_log2_t.requires_grad_(False)

    def set_train_log2(self):
        self.acti_log2_t.requires_grad_(True)

    def relu_forward(self, input):
        # if a computation is split into multiple crossbars
        if PIMUnit.quant_phase == PIMUnit.QuantPhase.INFERENCE:
            for i in range(len(input)):
                input[i] = self.pim_unit.quant_acti(F.relu(input[i]), False)
            input = sum(input)
        else:
            input = self.pim_unit.quant_acti(F.relu(input), False)
        return input

    def forward(self, input):
        if PIMUnit.quant_phase == PIMUnit.QuantPhase.FLOAT:
            return F.relu(input)
        else:
            return self.relu_forward(input)

    def share(self, average):
        self.acti_log2_t.data = average
