import torch
import torch.nn as nn


def threshold_weight_3sd(module, eps=1e-8):
    mean_value = module.weight.mean().abs().data
    #mean_value = torch.tensor([mean_value], device=mean_value.get_device())
    mean_value = torch.tensor([mean_value])
    std_value = module.weight.std().data
    module.weight_log2_t = torch.nn.Parameter(torch.log2(mean_value + 3 * std_value))


def threshold_bias_3sd(module, eps=1e-8):
    mean_value = torch.tensor([module.bias.mean().abs().data])
    std_value = module.bias.std().data
    module.bias_log2_t = torch.nn.Parameter(torch.log2(mean_value + 3 * std_value))


def threshold_activation_3sd(module, eps=1e-8):
    mean_value = torch.tensor([module.hook_out.mean().abs().data])
    std_value = module.hook_out.std().data
    module.acti_log2_t = torch.nn.Parameter(torch.log2(mean_value + 3 * std_value))
