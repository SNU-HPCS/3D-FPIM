import torch
from enum import Enum


# set the target network (supports only 6 networks for now...)
NETWORK_TYPES=["ResNet18", 
               "ResNet50", 
               "VGG16", 
               "VGG19"]

best_acc = 0  # best test accuracy

# FIXME (for chkpt path)
CHECKPOINT_PATH='./checkpoint/imagenet'

#FIXME (for network type)
TYPE= 2
NETWORK=NETWORK_TYPES[TYPE]

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

# FIXME (for retraining mode)
MODE=MODE_TYPE.BASELINEQUANT
#MODE=MODE_TYPE.FOLDINGBN
#MODE=MODE_TYPE.QACONV
#MODE=MODE_TYPE.INFERENCE

# FIXME (for gpu and mini-batch settings)
MULTI_GPU=True
#DEVICE_IDS=[2,3,4]
DEVICE_IDS=[0,1]
#DEVICE_IDS=[2,3,4,5,6]
#DEVICE_IDS=[1,2,3,4,5,6,7]
GPU_ID=DEVICE_IDS[0]
RETRAIN_BATCH=128
TEST_BATCH=128

# FIXME (for VGG-type calibration
CALIB_BATCH=256


device = 'cuda:' + str(GPU_ID) if torch.cuda.is_available() else 'cpu'
