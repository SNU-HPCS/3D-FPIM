from collections import namedtuple
import torch.nn as nn
from .fold import ShareQuant, Conv2dBNReLU, Conv2dBN
from .function import Conv2d, Linear
from .hardware import PIMUnit
import torch


# configuring the bitwidth of the weight / bias / activation
Config = namedtuple('config', ['weight', 'bias', 'acti'], defaults=[5, 12, 5])

# configure the bitwidth of the whole network
def config_network(config):

    PIMUnit.acti_bit_width = config.acti
    PIMUnit.bias_bit_width = config.bias
    PIMUnit.weight_bit_width = config.weight

# configure the last layer of the network
def set_last(
        net,
        name,
        layer_list=[]):
    proc_list = list(net._modules.keys())
    if name in layer_list:
        net.last = True
    for n in proc_list:
        set_last(net._modules[n],
                       name + '.' + n,
                       layer_list=layer_list)

def set_normalize(
        net,
        mode,
        name = ""):
    proc_list = list(net._modules.keys())
    if hasattr(net, "normalize"):
        net.normalize = mode
    for n in proc_list:
        set_normalize(net._modules[n], mode, name + '.' + n)

def profile_network(
        net,
        name = ""):
    proc_list = list(net._modules.keys())

    # profile only when
    # 1) not the pim unit (it is a typical layer)
    # 2) not the last layer
    if not isinstance(net, PIMUnit):
        
        if hasattr(net, "last") and net.last: pass
        else:
            if hasattr(net, "acti_log2_t"):
                if hasattr(net, "bn_frozen") and not getattr(net, "bn_frozen"):
                    print(name, "profile acti log2_t", "log2_t: ", \
                        net.relu.acti_log2_t.data, \
                        "bitwidth: ", net.pim_unit.acti_bit_width)
                else:
                    print(name, "profile acti log2_t", "log2_t: ", \
                        net.acti_log2_t.data, \
                        "bitwidth: ", net.pim_unit.acti_bit_width)
            if hasattr(net, "weight_log2_t"):
                if hasattr(net, "bn_frozen") and not getattr(net, "bn_frozen"):
                    print(name, "profile weight log2_t", "log2_t: ", \
                        net.conv.weight_log2_t.data, \
                        "bitwidth: ", net.pim_unit.weight_bit_width)
                else:
                    print(name, "profile weight log2_t", "log2_t: ", \
                        net.weight_log2_t.data, \
                        "bitwidth: ", net.pim_unit.weight_bit_width)
            if hasattr(net, "bias_log2_t"):
                if hasattr(net, "bn_frozen") and not getattr(net, "bn_frozen"):
                    if net.conv.bias != None:
                        print(name, "profile bias log2_t", "log2_t: ", \
                            net.conv.bias_log2_t.data, \
                            "bitwidth: ", net.pim_unit.bias_bit_width)
                else:
                    print(name, "profile bias log2_t", "log2_t: ", \
                        net.bias_log2_t.data, \
                        "bitwidth: ", net.pim_unit.bias_bit_width)
    if not isinstance(net, (Conv2dBNReLU, Conv2dBN)):
        for n in proc_list:
            profile_network(net._modules[n], name + '.' + n)

def init_log2_network(
        net,
        acti,
        weight,
        name = ""):
    proc_list = list(net._modules.keys())

    if hasattr(net, "acti_log2_t"):
        net.acti_log2_t.data = torch.tensor([acti], device=net.acti_log2_t.get_device())
    if hasattr(net, "weight_log2_t"):
        net.weight_log2_t.data = torch.tensor([weight], device=net.weight_log2_t.get_device())
    if hasattr(net, "bias_log2_t") and net.bias_log2_t != None:
        net.bias_log2_t.data = torch.tensor([weight], device=net.bias_log2_t.get_device())

    for n in proc_list:
        init_log2_network(net._modules[n], acti, weight, name + '.' + n)

def print_network(net):
    if isinstance(net, Conv2dBNReLU):
        return "Conv2dBNReLU"
    if isinstance(net, Conv2dBN):
        return "Conv2dBN"
    if isinstance(net, nn.ReLU):
        return "ReLU"
    if isinstance(net, ShareQuant):
        return "ShareQuant"
    if isinstance(net, Conv2d):
        return "Conv2d"
    if isinstance(net, Linear):
        return "Linear"


def print_log2_t_for_pim(
        net,
        prev_list,
        name = ""):
    proc_list = list(net._modules.keys())

    if hasattr(net, "acti_log2_t"):
        if net in prev_list:
            # The target net does not have preceding layer (First layer)
            if prev_list[net] != None:

                curr_acti_log2_t, prev_acti_log2_t, weight_log2_t, bias_log2_t = get_effective_log2_t(net, prev_list, name)
                if curr_acti_log2_t != None:
                    effective_log2_t = curr_acti_log2_t - prev_acti_log2_t - weight_log2_t
                    acti_wgt_log2_t = prev_acti_log2_t + weight_log2_t

                    print(name)
                    print("Required Weight X Acti Range", "[", PIMUnit.min_range_log2, ",", PIMUnit.max_range_log2 - PIMUnit.acti_bit_width,"]")
                    print("Current Weight X Acti Range: ", effective_log2_t)

                    if hasattr(net, "bias_log2_t"):
                        bias_lower_range = acti_wgt_log2_t + \
                                           (PIMUnit.bias_bit_width - \
                                           PIMUnit.acti_bit_width - \
                                           PIMUnit.weight_bit_width)
                        bias_upper_range = acti_wgt_log2_t + \
                                           (PIMUnit.bias_bit_width - \
                                           PIMUnit.weight_bit_width)
                        print("Required Bias Range", "[", bias_lower_range.item(), ",", bias_upper_range.item(), "]")
                        print("Current Bias Range: ", bias_log2_t)
                    #print(name, "Weight X Acti Validation ", PIMUnit.min_range_log2, "<=", effective_log2_t
                    #print(name, "ADC range: ", effective_log2_t, bias_lower_range, "<=", bias_log2_t, "<=", bias_upper_range)
    for n in proc_list:
        print_log2_t_for_pim(net._modules[n], prev_list, name + '.' + n)


def set_qa_conv_spec(net, precision, min_range_log2, max_range_log2, prev_list):
    
    assert(max_range_log2 >= precision + min_range_log2 and "incorrect minimum and maximum range")

    PIMUnit.precision = precision
    PIMUnit.min_range_log2 = min_range_log2
    PIMUnit.max_range_log2 = max_range_log2

    tune_log2_t_for_pim(net, prev_list)

def tune_log2_t_for_pim(net,
                        prev_list,
                        name = ""):
    proc_list = list(net._modules.keys())

    # clip bias according to the activation and weight log2_t
    def clip_bias(net, acti_wgt_log2_t):
        if torch.round(net.pim_unit.bias_log2_t).data < acti_wgt_log2_t + (PIMUnit.bias_bit_width - PIMUnit.acti_bit_width - PIMUnit.weight_bit_width):
            return acti_wgt_log2_t + (PIMUnit.bias_bit_width - PIMUnit.acti_bit_width - PIMUnit.weight_bit_width)
        elif torch.round(net.pim_unit.bias_log2_t).data > acti_wgt_log2_t + (PIMUnit.bias_bit_width - PIMUnit.weight_bit_width):
            return acti_wgt_log2_t + (PIMUnit.bias_bit_width - PIMUnit.weight_bit_width)
        else:
            return torch.round(net.pim_unit.bias_log2_t).data

    # only where ADC exists => Conv2dBNReLU, ReLU, ShareQuant
    if isinstance(net, (Conv2dBNReLU, nn.ReLU, ShareQuant)):
        if net in prev_list:
            if prev_list[net] != None:
                curr_acti_log2_t, prev_acti_log2_t, weight_log2_t, _ = get_effective_log2_t(net, prev_list, name)

                if curr_acti_log2_t != None:
                    effective_log2_t = curr_acti_log2_t - prev_acti_log2_t - weight_log2_t
                    acti_wgt_log2_t = prev_acti_log2_t + weight_log2_t
                
                if isinstance(net, Conv2dBNReLU):
                    if isinstance(prev_list[net], (Conv2dBN, Conv2dBNReLU, nn.ReLU, ShareQuant)):
                        if(effective_log2_t < PIMUnit.min_range_log2):
                            net.pim_unit.acti_log2_t.data -= effective_log2_t - PIMUnit.min_range_log2
                        elif(effective_log2_t + PIMUnit.acti_bit_width > PIMUnit.max_range_log2):
                            net.pim_unit.acti_log2_t.data -= (effective_log2_t + PIMUnit.acti_bit_width - PIMUnit.max_range_log2)

                # ReLU, ShareQuant
                if isinstance(net, (nn.ReLU, ShareQuant)):
                    if isinstance(prev_list[net], (Conv2dBN)):
                        if(effective_log2_t < PIMUnit.min_range_log2):
                            net.pim_unit.acti_log2_t.data -= effective_log2_t - PIMUnit.min_range_log2
                        elif(effective_log2_t + PIMUnit.acti_bit_width > PIMUnit.max_range_log2):
                            net.pim_unit.acti_log2_t.data -= (effective_log2_t + PIMUnit.acti_bit_width - PIMUnit.max_range_log2)
                
                # clamp the bias_log2_t according to the 
                curr_acti_log2_t, prev_acti_log2_t, weight_log2_t, _ = get_effective_log2_t(net, prev_list, name)

                if curr_acti_log2_t != None:
                    effective_log2_t = curr_acti_log2_t - prev_acti_log2_t - weight_log2_t
                    acti_wgt_log2_t = prev_acti_log2_t + weight_log2_t

                if isinstance(net, Conv2dBNReLU):
                    net.pim_unit.bias_log2_t.data = clip_bias(net, acti_wgt_log2_t)

                if curr_acti_log2_t != None:
                    # recheck if the previous value has been accurately clamped
                    assert(PIMUnit.min_range_log2 <= effective_log2_t and effective_log2_t + PIMUnit.acti_bit_width <= PIMUnit.max_range_log2)

    for n in proc_list:
        tune_log2_t_for_pim(net._modules[n], prev_list, name + '.' + n)

# Note that, this only works for ResNet and VGG-like networks
def get_effective_log2_t(net, prev_list, name):
    # 
    curr_acti_log2_t = None
    prev_acti_log2_t = None
    weight_log2_t = None
    bias_log2_t = None

    assert(prev_list[net] != None and "The first layer does not have a previous layer")
    if isinstance(net, (Conv2dBNReLU, Conv2dBN)):
        # the preceding layer should be either ReLU or ConvBNReLU
        assert(hasattr(prev_list[net], "acti_log2_t"))

        curr_acti_log2_t = torch.round(net.pim_unit.acti_log2_t).data
        prev_acti_log2_t = torch.round(prev_list[net].acti_log2_t).data
        weight_log2_t = torch.round(net.pim_unit.weight_log2_t).data
        bias_log2_t = torch.round(net.pim_unit.bias_log2_t).data

    if isinstance(net, (nn.ReLU, ShareQuant)):
        # in this case, we do not actually need a valid ADC => 
        # the computation is done in the digital domain
        if isinstance(prev_list[net], (Conv2dBN, nn.Linear)):

            curr_acti_log2_t = torch.round(net.pim_unit.acti_log2_t).data
            prev_acti_log2_t = torch.round(prev_list[prev_list[net]].pim_unit.acti_log2_t).data
            weight_log2_t = torch.round(prev_list[net].pim_unit.weight_log2_t).data
            bias_log2_t = torch.round(prev_list[net].pim_unit.bias_log2_t).data

    if curr_acti_log2_t != None:
        if(len(curr_acti_log2_t.data.shape) == 1): curr_acti_log2_t = curr_acti_log2_t.data[0]
        else: curr_acti_log2_t = curr_acti_log2_t.data

        if(len(prev_acti_log2_t.data.shape) == 1): prev_acti_log2_t = prev_acti_log2_t.data[0]
        else: prev_acti_log2_t = prev_acti_log2_t.data

        if(len(weight_log2_t.data.shape) == 1): weight_log2_t = weight_log2_t.data[0]
        else: weight_log2_t = weight_log2_t.data

        if(len(bias_log2_t.data.shape) == 1): bias_log2_t = bias_log2_t.data[0]
        else: bias_log2_t = bias_log2_t.data
        
    return curr_acti_log2_t, prev_acti_log2_t, weight_log2_t, bias_log2_t

def make_bn_freeze(net_proc, mode=True, convert=True, show=False, name = ""):
    keys = list(net_proc._modules.keys())
    if hasattr(net_proc, 'bn_freeze'):
        getattr(net_proc, 'bn_freeze')(mode, convert)
    for key in keys:
        make_bn_freeze(net_proc._modules[key],
                       mode=mode,
                       convert=convert,
                       show=show,
                       name = name + '.' + key)

def set_quant_phase(net_proc, quant_phase):
    keys = list(net_proc._modules.keys())
    if quant_phase == PIMUnit.QuantPhase.FLOAT or \
       quant_phase == PIMUnit.QuantPhase.QACONV or \
       quant_phase == PIMUnit.QuantPhase.INFERENCE:

        if hasattr(net_proc, 'set_fix_log2'):
            getattr(net_proc, 'set_fix_log2')()
    elif quant_phase == PIMUnit.QuantPhase.BASELINEQUANT or \
         quant_phase == PIMUnit.QuantPhase.FOLDINGBN:
        
        if hasattr(net_proc, 'set_train_log2'):
            getattr(net_proc, 'set_train_log2')()

    PIMUnit.quant_phase = quant_phase

    for key in keys:
        set_quant_phase(net_proc._modules[key], quant_phase)

