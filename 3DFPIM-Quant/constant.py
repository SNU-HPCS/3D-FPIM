import torchvision.models as models
from models import *
import config as cfg
import torch.backends.cudnn as cudnn

# network models


# should define
# 1) IMAGE_SIZE: dataset image size (for dataloader)
# 2) onet: original FP32 network (to retrieve the pretrained network)
# 3) qnet: retrained quantized network
# 4) qnet_profile: profiling the network for 
IMAGE_SIZE=None
onet=None
qnet=None
qnet_profile=None

model_name_q=None

first_layer=None
last_layer=None

def init_network():
    global IMAGE_SIZE
    global onet
    global model_name_q
    global qnet
    global qnet_profile

    global first_layer
    global last_layer
    # set the image size
    IMAGE_SIZE=224

    # set onet, qnet, qnet_profile
    if "ResNet18" in cfg.NETWORK:
        onet = models.resnet18(pretrained=True)
        model_name_q="ResNet18Q"
        qnet = ResNet18Q(num_classes=1000)
        qnet_profile = ResNet18Q(num_classes=1000)
    elif "ResNet50" in cfg.NETWORK:
        onet = models.resnet50(pretrained=True)
        model_name_q="ResNet50Q"
        qnet = ResNet50Q(num_classes=1000)
        qnet_profile = ResNet50Q(num_classes=1000)
    elif "ResNet101" in cfg.NETWORK:
        onet = models.resnet101(pretrained=True)
        model_name_q="ResNet101Q"
        qnet = ResNet101Q(num_classes=1000)
        qnet_profile = ResNet101Q(num_classes=1000)
    elif "ResNet152" in cfg.NETWORK:
        onet = models.resnet152(pretrained=True)
        model_name_q="ResNet152Q"
        qnet = ResNet152Q(num_classes=1000)
        qnet_profile = ResNet152Q(num_classes=1000)
    elif "VGG16" in cfg.NETWORK:
        onet = models.vgg16_bn(pretrained=True)
        model_name_q="VGGQ16"
        qnet = VGGQ("VGGQ16", num_classes=1000)
        qnet_profile = VGGQ("VGGQ16", num_classes=1000)
    elif "VGG19" in cfg.NETWORK:
        onet = models.vgg19_bn(pretrained=True)
        model_name_q="VGGQ19"
        qnet = VGGQ("VGGQ19", num_classes=1000)
        qnet_profile = VGGQ("VGGQ19", num_classes=1000)
    else:
        raise("Not Implemented")

    if "ResNet" in cfg.NETWORK:
        first_layer=[".conv1", ".bn1", ".relu"]
        #first_layer=[".share1"]
        last_layer=[".fc"]
    elif "VGG" in cfg.NETWORK:
        first_layer=[".features.0", ".features.1", ".features.2"]
        #first_layer=[".share1"]
        last_layer=[".classifier.6"]
    else:
        raise("Not Implemented")

    onet = onet.to(cfg.device)
    qnet = qnet.to(cfg.device)
    qnet_profile = qnet_profile.to('cpu')
    if cfg.MULTI_GPU:
        qnet = torch.nn.DataParallel(qnet, device_ids=cfg.DEVICE_IDS)
