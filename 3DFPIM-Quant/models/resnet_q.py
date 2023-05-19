import torch
import sys 

sys.path.append('../qa_conv')

import qa_conv.function as nn
import qa_conv

def conv3x3(in_planes: int, out_planes: int, stride: int = 1, groups: int = 1, dilation: int = 1) -> nn.Conv2d:
    """3x3 convolution with padding"""
    return nn.Conv2d(in_planes, out_planes, kernel_size=3, stride=stride,
                     padding=dilation, groups=groups, bias=False, dilation=dilation)


def conv1x1(in_planes: int, out_planes: int, stride: int = 1) -> nn.Conv2d:
    """1x1 convolution"""
    return nn.Conv2d(in_planes, out_planes, kernel_size=1, stride=stride, bias=False)


class BasicBlock(nn.Module):
    expansion = 1

    # the prev_list => for the folded network
    def __init__(self, in_planes, planes, stride=1, groups=1, base_width= 64, prev_layer = None, prev_list = None):
        super(BasicBlock, self).__init__()
        if groups != 1 or base_width != 64:
            raise ValueError('BasicBlock only supports groups=1 and base_width=64')
        prev_layer_save = prev_layer
        self.conv1 = nn.Conv2d(in_planes, planes, kernel_size=3, stride=stride, padding=1, bias=False)
        prev_list[self.conv1] = prev_layer

        self.bn1 = torch.nn.BatchNorm2d(planes)
        self.relu1 = nn.ReLU()
        prev_layer = self.relu1
        
        self.conv2 = nn.Conv2d(planes, planes, kernel_size=3, stride=1, padding=1, bias=False)
        prev_list[self.conv2] = prev_layer
        prev_layer = self.conv2
        
        self.bn2 = torch.nn.BatchNorm2d(planes)

        self.downsample = None
        if stride != 1 or in_planes != self.expansion*planes:
            self.downsample = torch.nn.Sequential(
                nn.Conv2d(in_planes, self.expansion*planes,kernel_size=1, stride=stride, bias=False), 
                torch.nn.BatchNorm2d(self.expansion*planes)
            )
            prev_list[self.downsample[0]] = prev_layer_save
            prev_layer_save = self.downsample[0]
        
        self.share1 = qa_conv.fold.ShareQuant()
        prev_list[self.share1] = prev_layer_save
        
        self.share2 = qa_conv.fold.ShareQuant()
        prev_list[self.share2] = prev_layer

        self.relu2 = nn.ReLU()
        prev_list[self.relu2] = self.share2

    def last_layer(self):
        return self.relu2

    #def share_path(self, show=False):
    #    data = torch.stack([torch.mean(torch.stack([self.share1.acti_log2_t.data, \
    #                                   self.share2.acti_log2_t.data]))])
    #    self.share1.share(data)
    #    self.share2.share(data)

    def forward(self, x):
        identity = x

        out = self.conv1(x)
        out = self.bn1(out)
        out = self.relu1(out)

        out = self.conv2(out)
        out = self.bn2(out)

        if self.downsample is not None:
            identity = self.downsample(x)
        
        out = self.share1(identity) + self.share2(out)
        out = self.relu2(out)

        return out

class Bottleneck(nn.Module):
    expansion = 4

    def __init__(self, in_planes, planes, stride=1, groups=1, base_width= 64, prev_layer = None, prev_list = None):
        super(Bottleneck, self).__init__()
        width = int(planes * (base_width / 64.)) * groups

        prev_layer_save = prev_layer
        self.conv1 = nn.Conv2d(in_planes, width, kernel_size=1, bias=False)
        prev_list[self.conv1] = prev_layer

        self.bn1 = torch.nn.BatchNorm2d(width)
        self.relu1 = nn.ReLU()
        prev_layer = self.relu1

        self.conv2 = nn.Conv2d(width, width, kernel_size=3, stride=stride, groups=groups, padding=1, bias=False)
        prev_list[self.conv2] = prev_layer

        self.bn2 = torch.nn.BatchNorm2d(width)
        self.relu2 = nn.ReLU()
        prev_layer = self.relu2

        self.conv3 = nn.Conv2d(width, self.expansion * planes, kernel_size=1, bias=False)
        prev_list[self.conv3] = prev_layer
        prev_layer = self.conv3

        self.bn3 = torch.nn.BatchNorm2d(self.expansion*planes)

        self.downsample = None
        if stride != 1 or in_planes != self.expansion*planes:
            self.downsample = nn.Sequential(
                nn.Conv2d(in_planes, self.expansion*planes, kernel_size=1, stride=stride, bias=False),
                torch.nn.BatchNorm2d(self.expansion*planes)
            )
            prev_list[self.downsample[0]] = prev_layer_save
            prev_layer_save = self.downsample[0]

        self.share1 = qa_conv.fold.ShareQuant()
        prev_list[self.share1] = prev_layer_save

        self.share2 = qa_conv.fold.ShareQuant()
        prev_list[self.share2] = prev_layer

        self.relu3 = nn.ReLU()
        prev_list[self.relu3] = self.share2

    def last_layer(self):
        return self.relu3

    #def share_path(self, show=False):
    #    data = torch.stack([torch.mean(self.share1.acti_log2_t.data, \
    #                                   self.share2.acti_log2_t.data, \
    #                                   self.relu3.acti_log2_t.data)])
    #    self.share1.share(data)
    #    self.share2.share(data)
    #    self.relu3.share(data)

    def forward(self, x):
        identity = x

        out = self.conv1(x)
        out = self.bn1(out)
        out = self.relu1(out)

        out = self.conv2(out)
        out = self.bn2(out)
        out = self.relu2(out)

        out = self.conv3(out)
        out = self.bn3(out)

        if self.downsample is not None:
            identity = self.downsample(x)

        out = self.share1(identity) + self.share2(out)
        out = self.relu3(out)
    
        return out


class ResNetQ(nn.Module):
    def __init__(self, block, num_blocks, groups=1, width_per_group=64, num_classes=10):
        super(ResNetQ, self).__init__()

        self.prev_list = {}
        prev_layer = None

        self.num_classes = num_classes
        self.in_planes = 64

        ##
        self.groups = groups
        self.base_width = width_per_group

        #self.share1 = qa_conv.fold.ShareQuant()

        #self.prev_list[self.share1] = prev_layer
        #prev_layer = self.share1

        if num_classes == 10:
            self.conv1 = nn.Conv2d(3, 64, kernel_size=3, stride=1, padding=1, bias=False)
        else:
            self.conv1 = nn.Conv2d(3, 64, kernel_size=7, stride=2, padding=3, bias=False)
        self.prev_list[self.conv1] = prev_layer

        self.bn1 = torch.nn.BatchNorm2d(64)
        self.relu = nn.ReLU()
        prev_layer = self.relu

        if num_classes == 1000:
            self.maxpool = torch.nn.MaxPool2d(3, stride=2, padding=1)
        self.layer1, prev_layer = self._make_layer(block, 64, num_blocks[0], stride=1, prev_layer = prev_layer)
        self.layer2, prev_layer = self._make_layer(block, 128, num_blocks[1], stride=2, prev_layer = prev_layer)
        self.layer3, prev_layer = self._make_layer(block, 256, num_blocks[2], stride=2, prev_layer = prev_layer)
        self.layer4, prev_layer = self._make_layer(block, 512, num_blocks[3], stride=2, prev_layer = prev_layer)
        if num_classes == 10:
            self.avgpool = torch.nn.AvgPool2d((4, 4))
            self.fc = torch.nn.Linear(512*block.expansion, num_classes)
        else:
            self.avgpool = torch.nn.AdaptiveAvgPool2d((1, 1))
            self.fc = torch.nn.Linear(512*block.expansion, num_classes)
        self.prev_list[self.fc] = prev_layer

    def _make_layer(self, block, planes, num_blocks, stride, prev_layer):
        strides = [stride] + [1]*(num_blocks-1)
        layers = []
        for stride in strides:
            layers.append(block(self.in_planes, planes, stride, self.groups, self.base_width, prev_layer, self.prev_list))
            prev_layer = layers[-1].last_layer()
            self.in_planes = planes * block.expansion
        return torch.nn.Sequential(*layers), prev_layer

    def forward(self, x):

        #out = self.share1(x)

        out = self.conv1(x)
        out = self.bn1(out)
        out = self.relu(out)
        if self.num_classes == 1000:
            out = self.maxpool(out)

        out = self.layer1(out)
        out = self.layer2(out)
        out = self.layer3(out)
        out = self.layer4(out)

        out = self.avgpool(out)
        out = out.view(out.size(0), -1)
        out = self.fc(out)
        return out


def ResNet18Q(num_classes=10):
    return ResNetQ(BasicBlock, [2, 2, 2, 2], num_classes=num_classes)


def ResNet34Q(num_classes=10):
    return ResNetQ(BasicBlock, [3, 4, 6, 3], num_classes=num_classes)


def ResNet50Q(num_classes=10):
    return ResNetQ(Bottleneck, [3, 4, 6, 3], num_classes=num_classes)


def ResNet101Q(num_classes=10):
    return ResNetQ(Bottleneck, [3, 4, 23, 3], num_classes=num_classes)

def ResNet152Q(num_classes=10):
    return ResNetQ(Bottleneck, [3, 8, 36, 3], num_classes=num_classes)

def WideResNet50Q_2(num_classes=10):
    return ResNetQ(Bottleneck, [3, 4, 6, 3], width_per_group=64*2, num_classes=num_classes)

def WideResNet101Q_2(num_classes=10):
    return ResNetQ(Bottleneck, [3, 4, 23, 3], width_per_group=64*2, num_classes=num_classes)

