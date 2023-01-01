from ..function.extra import SEConv2d, Adder2d


def isShiftAdder(module):
    return isinstance(module[0], SEConv2d) and isinstance(module[1], Adder2d)
