import torch.nn as nn


class _FoldModule(nn.Module):
    def __init__(self):
        super().__init__()
        
def isfoldmodule(x):
    return isinstance(x, _FoldModule)