import torch
from enum import Enum

best_acc = 0  # best test accuracy

# set the target network (supports only 6 networks for now...)
NETWORK_TYPES=["ResNet18", 
               "ResNet50", 
               "VGG16", 
               "VGG19"]

#FIXME (for network type)
TYPE= 0
NETWORK=NETWORK_TYPES[TYPE]

# FIXME (for chkpt path)
CHECKPOINT_PATH='./checkpoint/imagenet'

# Mode setting
class MODE_TYPE(Enum):
    # training w/ batchnorm from the scratch (w/ batchnorm)
    BASELINEQUANT = 0
    # fold the batch norm to the preceding layer
    # (we only support folding to the preceding layer for now...)
    FOLDINGBN = 1
    # qa_conv retraining (involves setting the quantization range according to the ADC spec)
    QACONV = 2
    # inference qa_conv
    INFERENCE = 3

# FIXME
MODE=MODE_TYPE.BASELINEQUANT

# FIXME
MULTI_GPU=True
DEVICE_IDS=[0,1,2,3,4,5,6,7]
GPU_ID=DEVICE_IDS[0]

# FIXME (set the batch size)
RETRAIN_BATCH=128
TEST_BATCH=128

# FIXME (set how may samples to use to profile the initial threshold values)
CALIB_BATCH=256

# FIXME (set the epoch end)
epoch_end = 110

# FIXME: set the weight, bias, and activate bitwidth
## CONFIG: bitwidths for (weight, bias, activation)
# we assume QLC cell and use even and odd row for signed 5-bit
# for bias, we use the whole array; therefore, we can use 128 rows to represent a single bias
# 128 rows => 2**7 => 7 additional bits for bias
WGT_BIT = 5
BIAS_BIT = 12
ACT_BIT = 5

device = 'cuda:' + str(GPU_ID) if torch.cuda.is_available() else 'cpu'
