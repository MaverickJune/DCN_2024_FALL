import torch
import torch.nn as nn
import torch.nn.functional as F
import torchvision.transforms as transforms
from torchvision import transforms, datasets
from PIL import Image

import os

SAVE_PATH = "./model_weight"
KERNEL_SIZE = 3

class CNNModel(nn.Module):
    def __init__(self):
        super(CNNModel, self).__init__()
        
        # Convolutional Layers
        self.conv1 = nn.Conv2d(in_channels=3, out_channels=32, kernel_size=KERNEL_SIZE, padding=1)  # Assuming RGB input
        self.bn1 = nn.BatchNorm2d(32)
        
        self.conv2 = nn.Conv2d(in_channels=32, out_channels=32, kernel_size=KERNEL_SIZE, padding=1)
        self.bn2 = nn.BatchNorm2d(32)
        
        self.conv3 = nn.Conv2d(in_channels=32, out_channels=64, kernel_size=KERNEL_SIZE, padding=1)
        self.bn3 = nn.BatchNorm2d(64)
        
        self.conv4 = nn.Conv2d(in_channels=64, out_channels=64, kernel_size=KERNEL_SIZE, padding=1)
        self.bn4 = nn.BatchNorm2d(64)
        
        self.conv5 = nn.Conv2d(in_channels=64, out_channels=128, kernel_size=KERNEL_SIZE, padding=1)
        self.bn5 = nn.BatchNorm2d(128)
        
        self.conv6 = nn.Conv2d(in_channels=128, out_channels=128, kernel_size=KERNEL_SIZE, padding=1)
        self.bn6 = nn.BatchNorm2d(128)
        
        # Fully Connected Layers
        self.fc1 = nn.Linear(128 * 4 * 4, 128)  # Adjust input size based on the input shape and pooling
        self.fc2 = nn.Linear(128, 10)
        
        # Dropout
        self.dropout = nn.Dropout(0.25)

    def forward(self, x):
        # First Convolutional Block
        x = F.relu(self.bn1(self.conv1(x)))
        x = F.relu(self.bn2(self.conv2(x)))
        x = F.max_pool2d(x, 2)
        x = self.dropout(x)
        
        # Second Convolutional Block
        x = F.relu(self.bn3(self.conv3(x)))
        x = F.relu(self.bn4(self.conv4(x)))
        x = F.max_pool2d(x, 2)
        x = self.dropout(x)
        
        # Third Convolutional Block
        x = F.relu(self.bn5(self.conv5(x)))
        x = F.relu(self.bn6(self.conv6(x)))
        x = F.max_pool2d(x, 2)
        x = self.dropout(x)
        
        # Flatten
        x = torch.flatten(x, 1)
        
        # Fully Connected Layers
        x = F.relu(self.fc1(x))
        x = self.dropout(x)
        x = self.fc2(x)
        
        return F.softmax(x, dim=1)
    
    
class partialModel_1(nn.Module):
    '''
    TODO
    1. Define the class partialModel_1
    '''
        
        
class partialModel_2(nn.Module):
    '''
    TODO
    1. Define the class partialModel_2
    WARNING: The superset of the partial models should be the original model!
    '''
    

# load original model
pytorch_model = CNNModel()
'''
TODO
1. Load the weights of the model from the path "./model_weight"
'''
pytorch_model.eval()

# load partial model 1
partial_model_1 = partialModel_1()
'''
TODO
1. Load the weights of the model from the path "./model_weight"
'''
partial_model_1.eval()
    
# load partial model 2
partial_model_2 = partialModel_2()
'''
TODO
1. Load the weights of the model from the path "./model_weight"
'''
partial_model_2.eval()


# convert model to onnx model
'''
TODO
1. convert the pytorch_models to onnx models
HINT : use torch.onnx
https://pytorch.org/docs/stable/onnx.html
'''






