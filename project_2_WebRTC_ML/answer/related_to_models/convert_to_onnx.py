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
    def __init__(self):
        super(partialModel_1, self).__init__()
        
        # Define partial layers
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
        
        return x
        
class partialModel_2(nn.Module):
    def __init__(self):
        super(partialModel_2, self).__init__()
        
        # Define partial layers
        self.fc1 = nn.Linear(128 * 4 * 4, 128)  # Adjust input size based on the input shape and pooling
        self.fc2 = nn.Linear(128, 10)
        
        self.dropout = nn.Dropout(0.25)
        
    def forward(self, x):
        # Fully Connected Layers
        x = F.relu(self.fc1(x))
        x = self.dropout(x)
        x = self.fc2(x)
        
        return F.softmax(x, dim=1)
    

# load original model
pytorch_model = CNNModel()
for name, layer in pytorch_model.named_children():
    layer_path = f'./model_weight/{name}_weight.pth'
    layer.load_state_dict(torch.load(layer_path, weights_only=True))
pytorch_model.eval()

# load partial model 1
partial_model_1 = partialModel_1()
for name, layer in partial_model_1.named_children():
    layer_path = f'./model_weight/{name}_weight.pth'
    layer.load_state_dict(torch.load(layer_path, weights_only=True))
partial_model_1.eval()
    
# load partial model 2
partial_model_2 = partialModel_2()
for name, layer in partial_model_2.named_children():
    layer_path = f'./model_weight/{name}_weight.pth'
    layer.load_state_dict(torch.load(layer_path, weights_only=True))
partial_model_2.eval()


# convert model to onnx model
dummy_input_origin = torch.zeros(1, 3, 32, 32)
dummy_input_partial_1 = torch.zeros(1, 3, 32, 32)
dummy_input_partial_2 = torch.zeros(1, 128 * 4 * 4)

torch.onnx.export(pytorch_model, dummy_input_origin, 'onnx_model.onnx', verbose = True)
torch.onnx.export(partial_model_1, dummy_input_partial_1, 'onnx_model_partial_1.onnx', verbose = True)
torch.onnx.export(partial_model_2, dummy_input_partial_2, 'onnx_model_partial_2.onnx', verbose = True)







