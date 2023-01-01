import torch.nn as nn


def hook_handler(model, input, output):
    model.hook_out = output


def remove_hook(handles):
    for handle in handles:
        handle.remove()


def remove_hookout(layer, name, show=False):
    keys = list(layer._modules.keys())
    if keys == []:
        if show:
            print(name, 'has removed hookout')
        delattr(layer, 'hook_out')
    else:
        for key in keys:
            remove_hookout(layer._modules[key], name + '.' + key, show=show)


def add_hook(
        layer,
        name,
        hook_handler,
        end_list=(nn.Conv2d, nn.Linear, nn.ReLU, nn.BatchNorm2d),
        show=False):
    handles = []
    keys = list(layer._modules.keys())
    if isinstance(layer, end_list):
        if show:
            print(name, 'has added a hook', layer)
        handles.append(layer.register_forward_hook(hook_handler))
    for key in keys:
        handles.extend(
            add_hook(layer._modules[key],
                     name + '.' + key,
                     hook_handler,
                     end_list=end_list,
                     show=show))
    return handles


def get_hook(layer, name, show=False):
    hooks_got = []
    keys = list(layer._modules.keys())
    if hasattr(layer, 'hook_out'):
        hooks_got.append((name, layer.hook_out))
        if show:
            print(name, 'hook has been fetched')
    for key in keys:
        hooks_got.extend(
            get_hook(layer._modules[key], name + '.' + key, show=show))
    return hooks_got
