'''
Two different ADC structures are defined; conventional ADC and QA-ADC
The code 
'''

import torch
from torch.autograd import Function
import math
from torch.nn import Module

from enum import Enum

class PIMUnit(Module):
    # Crossbar-related Specs
    # we only consider error only when the inference is set to true
    program_error = 0
    num_rows = 128

    # ADC-related Specs
    precision = -1
    min_range_log2 = float('inf')
    max_range_log2 = float('-inf')
    # set the adc spec

    # bitwidth
    weight_bit_width = -1
    bias_bit_width = -1
    acti_bit_width = -1

    # quantization phase
    quant_phase = None

    class QuantPhase(Enum):
        FLOAT = 0
        BASELINEQUANT = 1
        FOLDINGBN = 2
        QACONV = 3
        INFERENCE = 4

    def __init__(self, target_layers):
        super(PIMUnit, self).__init__()

        self.target_layers = target_layers
        # set error
        self.weight_error = None
        self.bias_error = None

        if "weight" in target_layers:
            net = target_layers["weight"]
            if hasattr(net, "bn_frozen") and not getattr(net, "bn_frozen"):
                self.weight_log2_t = net.conv.weight_log2_t
            else:
                self.weight_log2_t = net.weight_log2_t

        if "bias" in target_layers:
            self.bias_log2_t = None
            net = target_layers["bias"]
            if hasattr(net, "bn_frozen") and not getattr(net, "bn_frozen"):
                self.bias_log2_t = net.conv.bias_log2_t
            else:
                self.bias_log2_t = net.bias_log2_t

        net = target_layers["acti"]
        self.acti_log2_t = net.acti_log2_t

    def set_adc_error(num_rows = 128, program_error = 0):
        PIMUnit.num_rows = num_rows
        self.weight_error = torch.tensor(np.random.normal(1, program_error, size=self.weight.shape), dtype=torch.float)
        self.bias_error = torch.tensor(np.random.normal(1, program_error, size=self.bias.shape), dtype=torch.float)

    #
    def quant_weight(self, input):
        round_en = PIMUnit.quant_phase == PIMUnit.QuantPhase.QACONV or PIMUnit.quant_phase == PIMUnit.QuantPhase.INFERENCE
        return signed_quant(input, self.weight_log2_t, self.weight_bit_width, round_en)

    def quant_bias(self, input):
        round_en = PIMUnit.quant_phase == PIMUnit.QuantPhase.QACONV or PIMUnit.quant_phase == PIMUnit.QuantPhase.INFERENCE
        return signed_quant(input, self.bias_log2_t, self.bias_bit_width, round_en)

    def quant_acti(self, input, signed):
        round_en = PIMUnit.quant_phase == PIMUnit.QuantPhase.QACONV or PIMUnit.quant_phase == PIMUnit.QuantPhase.INFERENCE
        if signed:
            # we are forced to sign
            out = signed_quant(input, self.acti_log2_t, self.acti_bit_width, round_en)
        else:
            out = unsigned_quant(input, self.acti_log2_t, self.acti_bit_width, round_en)
        return out


# set signed and unsigned quant method
@torch.jit.script
def signed_quant(tensor, log2_t, bit: int, round_en: bool):
    if round_en:
        alpha = (2 ** round(log2_t.detach()))
    else:
        alpha = 2 ** log2_t
    data = tensor / alpha
    data = data.clamp(-1, 1)
    data = data * (2 ** (bit - 1) - 1)
    data_q = (data.round() - data).detach() + data
    data_q = data_q / (2 ** (bit - 1) - 1) * alpha
    return data_q

@torch.jit.script
def unsigned_quant(tensor, log2_t, bit: int, round_en: bool):
    if round_en:
        alpha = (2 ** round(log2_t.detach()))
    else:
        alpha = 2 ** log2_t
    data = tensor / alpha
    data = data.clamp(0, 1)
    data = data * (2 ** bit - 1)
    data_q = (data.round() - data).detach() + data
    data_q = data_q / (2 ** bit - 1) * alpha
    return data_q

@torch.jit.script
def bn_norm(conv_weight, conv_bias, bn_weight, bn_var, bn_mean, bn_bias, eps: float):
    conv_weight = conv_weight * bn_weight / (bn_var + eps).sqrt()
    conv_bias = bn_weight * (conv_bias - bn_mean) / (bn_var + eps).sqrt() + bn_bias
    return conv_weight, conv_bias
