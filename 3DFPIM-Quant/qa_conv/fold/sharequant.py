import torch
import torch.nn as nn

from ..hardware import PIMUnit


class ShareQuant(nn.Module):

    def __init__(self):
        super().__init__()
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

    def share(self, average):
        self.acti_log2_t.data = average

    def share_forward(self, input):
        if PIMUnit.quant_phase == PIMUnit.QuantPhase.INFERENCE:
            for i in range(len(input)):
                input[i] = self.pim_unit.quant_acti(input[i], True)
            input = sum(input)
        else:
            input = self.pim_unit.quant_acti(input, True)
        return input

    def forward(self, input):
        if PIMUnit.quant_phase == PIMUnit.QuantPhase.FLOAT:
            return input
        else:
            return self.share_forward(input)
