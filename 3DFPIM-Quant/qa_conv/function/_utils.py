import torch


def number_to_tensor(x, t):
    r'''
    Turn x in to a tensor with data type like tensor t.
    '''
    return torch.tensor(x).type_as(t)