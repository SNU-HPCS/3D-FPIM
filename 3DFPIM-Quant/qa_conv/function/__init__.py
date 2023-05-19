from torch.nn import Module, ModuleList, ModuleDict, Sequential

from torch.nn import Parameter

# from torch import init
# from torch import utils

from torch.nn import MaxPool1d, MaxPool2d, MaxPool3d
from torch.nn import AvgPool1d, AvgPool2d, AvgPool3d, AdaptiveAvgPool2d

from .activation import ReLU
from .conv import Conv2d
from .linear import Linear

from . import extra
from .extra import SEConv2d

from .. import hardware
