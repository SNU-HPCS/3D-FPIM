import torch
import torch.nn as nn
from collections import OrderedDict
from .convbnact import Conv2dBNReLU
from .convbn import Conv2dBN
from .sharequant import ShareQuant
from ..hardware import PIMUnit

def fold_CBR(conv, bn, relu, prev_list = None):
    folded = Conv2dBNReLU(conv, bn, relu)
    conv_new = folded
    bn_new = nn.Identity()
    relu_new = nn.Identity()

    if prev_list != None:
        for key in list(prev_list.keys()):
            value = prev_list[key]
            if value == conv or value == relu:
                prev_list[key] = folded
            if key == conv or key == relu:
                prev_list[folded] = prev_list[key]
                del prev_list[key]

    conv = conv_new
    bn = bn_new
    relu = relu_new

    return conv, bn, relu

def fold_CB(conv, bn, prev_list = None):
    folded = Conv2dBN(conv, bn)
    conv_new = folded
    bn_new = nn.Identity()

    if prev_list != None:
        for key in list(prev_list.keys()):
            value = prev_list[key]
            if value == conv:
                prev_list[key] = folded
            if key == conv:
                prev_list[folded] = prev_list[key]
                del prev_list[key]

    conv = conv_new
    bn = bn_new

    return conv, bn

def fold_the_convnet(net, prev_list = None):
    key = list(net._modules.keys())
    keylen = len(key)
    # for convbn
    flag = 0
    while flag < keylen:
        if list(net._modules[key[flag]]._modules.keys()) != []:
            fold_the_network(net._modules[key[flag]], prev_list = prev_list)
        else:
            if isinstance(net._modules[key[flag]],
                          nn.Conv2d) and flag + 1 < keylen:
                if isinstance(net._modules[key[flag + 1]],
                              nn.BatchNorm2d) and flag + 2 < keylen:
                    if isinstance(net._modules[key[flag + 2]], nn.ReLU):
                        # Generate CBR
                        net._modules[key[flag]], net._modules[key[
                            flag + 1]], net._modules[key[flag + 2]] = fold_CBR(
                                net._modules[key[flag]],
                                net._modules[key[flag + 1]],
                                net._modules[key[flag + 2]], prev_list = prev_list)
                        flag += 2
                    else:
                        # Generate CB
                        net._modules[key[flag]], net._modules[key[
                            flag + 1]] = fold_CB(net._modules[key[flag]],
                                                 net._modules[key[flag + 1]], prev_list = prev_list)
                        flag += 1
                elif isinstance(net._modules[key[flag + 1]], nn.BatchNorm2d):
                    # Generate CB
                    net._modules[key[flag]], net._modules[key[
                        flag + 1]] = fold_CB(net._modules[key[flag]],
                                             net._modules[key[flag + 1]], prev_list = prev_list)
                    flag += 1
        flag += 1
    # declare

# set the pim units for the layers
def init_pim_units(net, prev_list, name = ""):
    proc_list = list(net._modules.keys())

    # set the pim unit according to the ADC
    if net in prev_list.keys():
        if isinstance(net, Conv2dBNReLU):
            target_layers = {"acti" : net, "weight" : net, "bias" : net}
            pim_unit = PIMUnit(target_layers = target_layers)
            assert(hasattr(net, "set_pim_unit") and "set_pim_unit undefined for your custom layer")
            net.set_pim_unit(pim_unit)

        # A set of layers are mapped to a single PIM
        elif isinstance(net, (ShareQuant, nn.ReLU)):
            # requires a mixed-signal operation
            if isinstance(prev_list[net], (Conv2dBN)):
                target_layers = {"acti" : net, "weight" : prev_list[net], "bias" : prev_list[net]}
                pim_unit = PIMUnit(target_layers = target_layers)
                assert(hasattr(net, "set_pim_unit") and "set_pim_unit undefined for your custom layer")
                net.set_pim_unit(pim_unit)
                assert(hasattr(prev_list[net], "set_pim_unit") and "set_pim_unit undefined for your custom layer")
                prev_list[net].set_pim_unit(pim_unit)
            elif isinstance(prev_list[net], (nn.Conv2d, nn.Linear)):
                target_layers = {"acti" : net, "weight" : prev_list[net], "bias" : prev_list[net]}
                pim_unit = PIMUnit(target_layers = target_layers)
                assert(hasattr(net, "set_pim_unit") and "set_pim_unit undefined for your custom layer")
                net.set_pim_unit(pim_unit)
                assert(hasattr(prev_list[net], "set_pim_unit") and "set_pim_unit undefined for your custom layer")
                prev_list[net].set_pim_unit(pim_unit)
            # requires a digital-only ALU operation
            else:
                target_layers = {"acti" : net}
                pim_unit = PIMUnit(target_layers = target_layers)
                assert(hasattr(net, "set_pim_unit") and "set_pim_unit undefined for your custom layer")
                net.set_pim_unit(pim_unit)

    for n in proc_list:
        init_pim_units(net._modules[n], prev_list, name + '.' + n)



def fold_the_network(net, prev_list, fold_type='conv'):
    if fold_type == 'conv':
        fold_the_convnet(net, prev_list = prev_list) 
    else:
        raise NotImplementedError()
    init_pim_units(net, prev_list)
