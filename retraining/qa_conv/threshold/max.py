import torch
import torch.nn as nn


def threshold_weight_max(module, eps=1e-8):
    max_value = module.weight.abs().flatten().max().data
    #max_value = torch.tensor([max_value], device=max_value.get_device()) + eps
    max_value = torch.tensor([max_value]) + eps
    module.weight_log2_t = torch.nn.Parameter(torch.log2(max_value))


def threshold_bias_max(module, eps=1e-8):
    max_value = torch.tensor([module.bias.abs().flatten().max().data]) + eps
    module.bias_log2_t = torch.nn.Parameter(torch.log2(max_value))


def threshold_activation_max(module, eps=1e-8):
    max_value = torch.tensor([module.hook_out.abs().flatten().max().data
                              ]) + eps
    module.acti_log2_t = torch.nn.Parameter(torch.log2(max_value))
