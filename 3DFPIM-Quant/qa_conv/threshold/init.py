from .max import *
from .sd import *
from .kl import *

import sys

from ..function import Conv2d, Linear, ReLU
from ..fold import ShareQuant, Conv2dBNReLU, Conv2dBN


def init_weight(net_module, method='max'):
    if method == 'max':
        threshold_weight_max(net_module)
    elif method == '3sd':
        threshold_weight_3sd(net_module)
    else:
        raise NotImplementedError()


def init_bias(net_module, method='max'):
    if method == 'max':
        threshold_bias_max(net_module)
    elif method == '3sd':
        threshold_bias_3sd(net_module)
    else:
        raise NotImplementedError()


def init_acti(net_module,
              method='entro',
              bin_number=2048,
              cali_number=128):
    if method == 'entro':
        entropy_calibration(net_module,
                            bin_number=bin_number,
                            cali_number=cali_number)
    elif method == 'max':
        threshold_activation_max(net_module)
    else:
        raise NotImplementedError()


def init_network(net_proc,
                 name,
                 weight_method='max',
                 bias_method='max',
                 acti_method='entro',
                 bin_number=2048,
                 cali_number=128,
                 show=False):
    modules = list(net_proc._modules.keys())


    if modules == [] and isinstance(net_proc, (Conv2d, Linear, ReLU, ShareQuant, Conv2dBNReLU, Conv2dBN)):
        if net_proc.layer_data_type["weight"]:
            #if net_proc.quant is True:
            init_weight(net_proc, method=weight_method)
            if show:
                print(name, ': weight threshold is quanted as',
                      float(net_proc.weight_log2_t))
        if net_proc.layer_data_type["bias"] and net_proc.bias is not None:
            #if net_proc.quant is True:
            init_bias(net_proc, method=bias_method)
            if show:
                print(name, ': bias threshold is quanted as',
                      float(net_proc.bias_log2_t))
        if net_proc.layer_data_type["acti"]:
            #if net_proc.quant is True:
            init_acti(net_proc,
                      method=acti_method,
                      bin_number=bin_number,
                      cali_number=cali_number)
            if show:
                print(name, ': activation threshold is quanted as',
                      float(net_proc.acti_log2_t))
        sys.stdout.flush()
    else:
        for key in modules:
            init_network(net_proc._modules[key],
                         name + '.' + key,
                         weight_method=weight_method,
                         bias_method=bias_method,
                         acti_method=acti_method,
                         bin_number=bin_number,
                         cali_number=cali_number,
                         show=show)
