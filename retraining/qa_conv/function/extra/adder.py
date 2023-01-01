"""
Provide Adder2d, https://arxiv.org/pdf/1912.13200.pdf
"""

import torch
import torch.nn as nn
import torch.nn.functional as F
from torch.autograd import Function
import math


class Adder(Function):
    @staticmethod
    def forward(ctx, W_col, X_col):
        ctx.save_for_backward(W_col, X_col)
        output = -(W_col.unsqueeze(2) - X_col.unsqueeze(0)).abs().sum(1)
        return output

    @staticmethod
    def backward(ctx, grad_output):
        W_col, X_col = ctx.saved_tensors
        grad_W_col = ((X_col.unsqueeze(0) - W_col.unsqueeze(2)) *
                      grad_output.unsqueeze(1)).sum(2)
        grad_W_col = grad_W_col / grad_W_col.norm(p=2).clamp(
            min=1e-12) * math.sqrt(W_col.size(1) * W_col.size(0)) / 5
        grad_X_col = (-(X_col.unsqueeze(0) - W_col.unsqueeze(2)).clamp(-1, 1) *
                      grad_output.unsqueeze(1)).sum(0)

        return grad_W_col, grad_X_col


adder = Adder.apply


def adder2d_function(X, W, stride=1, padding=0):
    n_filters, d_filter, h_filter, w_filter = W.size()
    n_x, d_x, h_x, w_x = X.size()

    h_out = (h_x - h_filter + 2 * padding) / stride + 1
    w_out = (w_x - w_filter + 2 * padding) / stride + 1

    h_out, w_out = int(h_out), int(w_out)
    X_col = torch.nn.functional.unfold(X.view(1, -1, h_x, w_x),
                                       h_filter,
                                       dilation=1,
                                       padding=padding,
                                       stride=stride).view(
                                           n_x, -1, h_out * w_out)
    X_col = X_col.permute(1, 2, 0).contiguous().view(X_col.size(1), -1)
    W_col = W.view(n_filters, -1)

    out = adder(W_col, X_col)

    out = out.view(n_filters, h_out, w_out, n_x)
    out = out.permute(3, 0, 1, 2).contiguous()

    return out


class Adder2d(nn.Module):
    def __init__(self,
                 input_channel,
                 output_channel,
                 kernel_size,
                 bias=False,
                 stride=1,
                 padding=0):
        super().__init__()
        self.stride = stride
        self.padding = padding
        self.input_channel = input_channel
        self.output_channel = output_channel
        self.kernel_size = kernel_size
        assert bias is False, f'Bias in adder function is of no use, remember to remove it.'
        self.weight = torch.nn.Parameter(
            nn.init.normal_(
                torch.randn(output_channel, input_channel, kernel_size,
                            kernel_size)))

    def forward(self, x):
        output = adder2d_function(x, self.weight, self.stride, self.padding)
        return output


if __name__ == '__main__':
    add = Adder2d(3, 4, 3)
    x = torch.rand(10, 3, 10, 10)
    print(add(x).shape)
