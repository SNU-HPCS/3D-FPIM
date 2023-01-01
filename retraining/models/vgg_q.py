'''VGG11/13/16/19 in Pytorch.'''
import torch
import sys 

sys.path.append('../qa_conv')

import qa_conv.function as nn
import qa_conv


cfg = {
    'VGGQ11': [64, 'M', 128, 'M', 256, 256, 'M', 512, 512, 'M', 512, 512, 'M'],
    'VGGQ13': [64, 64, 'M', 128, 128, 'M', 256, 256, 'M', 512, 512, 'M', 512, 512, 'M'],
    'VGGQ16': [64, 64, 'M', 128, 128, 'M', 256, 256, 256, 'M', 512, 512, 512, 'M', 512, 512, 512, 'M'],
    'VGGQ19': [64, 64, 'M', 128, 128, 'M', 256, 256, 256, 256, 'M', 512, 512, 512, 512, 'M', 512, 512, 512, 512, 'M'],
}


class VGGQ(nn.Module):
    def __init__(self, vgg_name, num_classes=10):
        super(VGGQ, self).__init__()
        self.prev_list = {}
        prev_layer = None

        #self.share1 = qa_conv.fold.ShareQuant()

        #self.prev_list[self.share1] = prev_layer
        #prev_layer = self.share1

        self.features, prev_layer = self._make_layers(cfg[vgg_name], prev_layer, num_classes)

        layers = []

        if num_classes == 10:
            layers += [nn.Linear(512, 512, bias=True)]
            self.prev_list[layers[-1]] = prev_layer
            prev_layer = layers[-1]

            layers += [nn.ReLU()]
            self.prev_list[layers[-1]] = prev_layer
            prev_layer = layers[-1]
            
            layers += [torch.nn.Dropout()]
            layers += [nn.Linear(512, 512, bias=True)]
            self.prev_list[layers[-1]] = prev_layer
            prev_layer = layers[-1]

            layers += [nn.ReLU()]
            self.prev_list[layers[-1]] = prev_layer
            prev_layer = layers[-1]
            
            layers += [torch.nn.Dropout()]
            layers += [nn.Linear(512, 10, bias=True)]
            self.prev_list[layers[-1]] = prev_layer
            prev_layer = layers[-1]

        else:
            layers += [nn.Linear(7*7*512, 4096, bias=True)]
            self.prev_list[layers[-1]] = prev_layer
            prev_layer = layers[-1]

            layers += [nn.ReLU()]
            self.prev_list[layers[-1]] = prev_layer
            prev_layer = layers[-1]
            
            layers += [torch.nn.Dropout()]
            layers += [nn.Linear(4096, 4096, bias=True)]
            self.prev_list[layers[-1]] = prev_layer
            prev_layer = layers[-1]
            
            layers += [nn.ReLU()]
            self.prev_list[layers[-1]] = prev_layer
            prev_layer = layers[-1]

            layers += [torch.nn.Dropout()]
            layers += [nn.Linear(4096, num_classes, bias=True)]
            self.prev_list[layers[-1]] = prev_layer
            prev_layer = layers[-1]

        self.classifier = nn.Sequential(*layers)

    def forward(self, x):
        #out = self.share1(x)
        out = self.features(x)
        out = out.view(out.size(0), -1)
        out = self.classifier(out)
        return out

    def _make_layers(self, cfg, prev_layer, num_classes):
        layers = []
        in_channels = 3
        for x in cfg:
            if x == 'M':
                layers += [nn.MaxPool2d(kernel_size=2, stride=2)]
            else:
                temp_layers = [nn.Conv2d(in_channels, x, kernel_size=3, padding=1),
                           torch.nn.BatchNorm2d(x),
                           nn.ReLU(inplace=True)]
                
                self.prev_list[temp_layers[0]] = prev_layer
                prev_layer = temp_layers[2]

                layers += temp_layers
                in_channels = x
        if num_classes == 10:
            layers += [nn.AvgPool2d(kernel_size=1, stride=1)]
        else:
            layers += [nn.AdaptiveAvgPool2d((7, 7))]
        return nn.Sequential(*layers), prev_layer
