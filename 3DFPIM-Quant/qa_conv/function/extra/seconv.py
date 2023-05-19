import torch
import torch.nn as nn
import torch.nn.functional as F
import torch.nn.init as init
from torch.nn.modules.utils import _pair


class SEConv2d(nn.Module):
    def __init__(self,
                 in_channels,
                 out_channels,
                 kernel_size,
                 stride=1,
                 padding=0,
                 dilation=1,
                 groups=1,
                 bias=False,
                 size_splits=64,
                 threshold=5e-3,
                 sign_threshold=0.5,
                 distribution='uniform'):
        super(SEConv2d, self).__init__()
        if in_channels % groups != 0:
            raise ValueError('in_channels must be divisible by groups')
        self.in_channels = in_channels
        self.out_channels = out_channels
        self.kernel_size = _pair(kernel_size)
        self.stride = _pair(stride)
        self.padding = _pair(padding)
        self.dilation = _pair(dilation)
        self.groups = groups
        if bias:
            self.bias = nn.Parameter(torch.Tensor(out_channels))
        else:
            self.register_parameter('bias', None)

        self.weight = torch.nn.Parameter(
            nn.init.normal_(
                torch.randn(self.out_channels, self.in_channels, kernel_size,
                            kernel_size)))

    def forward(self, input):
        weight = self.weight.detach()
        output = F.conv2d(input, weight, self.bias, self.stride, self.padding,
                          self.dilation, self.groups)

        return output
