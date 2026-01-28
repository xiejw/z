# vim: ft=python
#
# This script is used to benchmark cpu vs mps with different batch sizes.
# Currently, we only test python code
#
# Results
#
# run_seconds,batch_size,iterations_per_run,device
# 0.150,  1,50,cpu
# 0.243,  2,50,cpu
# 0.355,  4,50,cpu
# 0.463,  8,50,cpu
# 2.380, 16,50,cpu
# 3.360, 32,50,cpu
# 3.990, 64,50,cpu
# 6.220,128,50,cpu
# 0.332,  1,50,mps
# 0.305,  2,50,mps
# 0.199,  4,50,mps
# 0.189,  8,50,mps
# 0.199, 16,50,mps
# 0.247, 32,50,mps
# 0.486, 64,50,mps
# 0.846,128,50,mps

import os

import numpy as np
import torch
import time


#
# initialize the env
#

def print_bar():
    print("")
    print("========================")
    print("")



class ResNetModel5x5(torch.nn.Module):

    # a residual for ResNet
    class ResidualAddLayer(torch.nn.Module):
        def __init__(self):
            super(ResNetModel5x5.ResidualAddLayer, self).__init__()

        def forward(self, x, inp):
            return x + inp

    def __init__(self):
        super(ResNetModel5x5, self).__init__()

        self.conv2d = torch.nn.Conv2d(
                in_channels=3,
                out_channels=128,
                kernel_size=(5, 5),
                padding='same')

        self.batch_n = torch.nn.BatchNorm2d(
                num_features=128,
                eps=0.001)

        self.relu    = torch.nn.ReLU(inplace=True)

        # block 0
        pos = 3
        self.b0_conv2d    =  torch.nn.Conv2d(
                in_channels=128,
                out_channels=128,
                kernel_size=(5, 5),
                padding='same')
        self.b0_batch_n   =  torch.nn.BatchNorm2d(
                num_features=128,
                eps=0.001)
        self.b0_relu      =  torch.nn.ReLU(inplace=True)
        self.b0_conv2d_b  =  torch.nn.Conv2d(
                in_channels=128,
                out_channels=128,
                kernel_size=(5, 5),
                padding='same')
        self.b0_batch_n_b =  torch.nn.BatchNorm2d(
                num_features=128,
                eps=0.001)
        self.b0_add       =  ResNetModel5x5.ResidualAddLayer()
        self.b0_relu_b    =  torch.nn.ReLU(inplace=True)

        # block 1
        pos = pos + 7
        self.b1_conv2d    =  torch.nn.Conv2d(
                    in_channels=128,
                    out_channels=128,
                    kernel_size=(5, 5),
                    padding='same')
        self.b1_batch_n   =  torch.nn.BatchNorm2d(
                num_features=128,
                eps=0.001)
        self.b1_relu      =  torch.nn.ReLU(inplace=True)
        self.b1_conv2d_b  =  torch.nn.Conv2d(
                    in_channels=128,
                    out_channels=128,
                    kernel_size=(5, 5),
                    padding='same')
        self.b1_batch_n_b = torch.nn.BatchNorm2d(
                num_features=128,
                eps=0.001)
        self.b1_add       =  ResNetModel5x5.ResidualAddLayer()
        self.b1_relu_b    =  torch.nn.ReLU(inplace=True)

        # block 2
        pos = pos + 7
        self.b2_conv2d    = torch.nn.Conv2d(
                    in_channels=128,
                    out_channels=128,
                    kernel_size=(5, 5),
                    padding='same')
        self.b2_batch_n   =  torch.nn.BatchNorm2d(
                num_features=128,
                eps=0.001)
        self.b2_relu      =  torch.nn.ReLU(inplace=True)
        self.b2_conv2d_b  = torch.nn.Conv2d(
                    in_channels=128,
                    out_channels=128,
                    kernel_size=(5, 5),
                    padding='same')
        self.b2_batch_n_b = torch.nn.BatchNorm2d(
                num_features=128,
                eps=0.001)
        self.b2_add       = ResNetModel5x5.ResidualAddLayer()
        self.b2_relu_b    =  torch.nn.ReLU(inplace=True)

        # block 3
        pos = pos + 7
        self.b3_conv2d    =  torch.nn.Conv2d(
                    in_channels=128,
                    out_channels=128,
                    kernel_size=(5, 5),
                    padding='same')
        self.b3_batch_n   = torch.nn.BatchNorm2d(
                num_features=128,
                eps=0.001)
        self.b3_relu      =  torch.nn.ReLU(inplace=True)
        self.b3_conv2d_b  =  torch.nn.Conv2d(
                    in_channels=128,
                    out_channels=128,
                    kernel_size=(5, 5),
                    padding='same')
        self.b3_batch_n_b = torch.nn.BatchNorm2d(
                num_features=128,
                eps=0.001)
        self.b3_add       =  ResNetModel5x5.ResidualAddLayer()
        self.b3_relu_b    =  torch.nn.ReLU(inplace=True)

        # block 4
        pos = pos + 7
        self.b4_conv2d    =  torch.nn.Conv2d(
                    in_channels=128,
                    out_channels=128,
                    kernel_size=(5, 5),
                    padding='same')
        self.b4_batch_n   =  torch.nn.BatchNorm2d(
                num_features=128,
                eps=0.001)
        self.b4_relu      =  torch.nn.ReLU(inplace=True)
        self.b4_conv2d_b  =  torch.nn.Conv2d(
                    in_channels=128,
                    out_channels=128,
                    kernel_size=(5, 5),
                    padding='same')
        self.b4_batch_n_b = torch.nn.BatchNorm2d(
                num_features=128,
                eps=0.001)
        self.b4_add       = ResNetModel5x5.ResidualAddLayer()
        self.b4_relu_b    =  torch.nn.ReLU(inplace=True)

        # policy head

        pos = pos + 7
        self.p_conv2d    =  torch.nn.Conv2d(
                    in_channels=128,
                    out_channels=2,
                    kernel_size=(1, 1),
                    padding='same')
        self.p_batch_n   =  torch.nn.BatchNorm2d(
                num_features=2,
                eps=0.001)
        self.p_relu      =  torch.nn.ReLU(inplace=True)
        self.p_flatten   =  torch.nn.Flatten()
        self.p_dense     =  torch.nn.Linear(
                    in_features=84,
                    out_features=42)
        self.p_softmax   =  torch.nn.Softmax(dim=1)

        # value head

        self.v_conv2d    =  torch.nn.Conv2d(
                    in_channels=128,
                    out_channels=2,
                    kernel_size=(1, 1),
                    padding='same')
        self.v_batch_n   =  torch.nn.BatchNorm2d(
                num_features=2,
                eps=0.001)
        self.v_relu      =  torch.nn.ReLU(inplace=True)
        self.v_flatten   =  torch.nn.Flatten()
        self.v_dense_1   =  torch.nn.Linear(
                    in_features=84,
                    out_features=256)
        self.v_relu      =  torch.nn.ReLU(inplace=True)
        self.v_dense_2   =  torch.nn.Linear(
                    in_features=256,
                    out_features=1)
        self.v_tanh      =  torch.nn.Tanh()


    def forward(self, x):
        x = self.conv2d  (x)
        x = self.batch_n (x)
        x = self.relu    (x)

        # block 0
        inp = x
        x = self.b0_conv2d    (x)
        x = self.b0_batch_n   (x)
        x = self.b0_relu      (x)
        x = self.b0_conv2d_b  (x)
        x = self.b0_batch_n_b (x)
        x = self.b0_add       (x, inp)
        x = self.b0_relu_b    (x)

        # block 1
        inp = x
        x = self.b1_conv2d    (x)
        x = self.b1_batch_n   (x)
        x = self.b1_relu      (x)
        x = self.b1_conv2d_b  (x)
        x = self.b1_batch_n_b (x)
        x = self.b1_add       (x, inp)
        x = self.b1_relu_b    (x)

        # block 2
        inp = x
        x = self.b2_conv2d    (x)
        x = self.b2_batch_n   (x)
        x = self.b2_relu      (x)
        x = self.b2_conv2d_b  (x)
        x = self.b2_batch_n_b (x)
        x = self.b2_add       (x, inp)
        x = self.b2_relu_b    (x)

        # block 3
        inp = x
        x = self.b3_conv2d    (x)
        x = self.b3_batch_n   (x)
        x = self.b3_relu      (x)
        x = self.b3_conv2d_b  (x)
        x = self.b3_batch_n_b (x)
        x = self.b3_add       (x, inp)
        x = self.b3_relu_b    (x)

        # block 4
        inp = x
        x = self.b4_conv2d    (x)
        x = self.b4_batch_n   (x)
        x = self.b4_relu      (x)
        x = self.b4_conv2d_b  (x)
        x = self.b4_batch_n_b (x)
        x = self.b4_add       (x, inp)
        x = self.b4_relu_b    (x)

        inp = x

        # policy head
        x = self.p_conv2d    (inp)
        x = self.p_batch_n   (x)
        x = self.p_relu      (x)
        x = self.p_flatten  (x)
        x = self.p_dense (x)
        x = self.p_softmax (x)

        p = x

        # value head
        x = self.v_conv2d    (inp)
        x = self.v_batch_n   (x)
        x = self.v_relu      (x)
        x = self.v_flatten   (x)
        x = self.v_dense_1   (x)
        x = self.v_relu      (x)
        x = self.v_dense_2   (x)
        x = self.v_tanh      (x)

        v = x

        return (p, v)

#
# do pytorch pred from new model
#
new_pt_m = ResNetModel5x5().eval()


def sync(device):
    if device == 'mps':
        torch._C._mps_synchronize

def run(m, batch_size, times_to_run, device, a):
    with torch.no_grad():
        m.to(device).eval()

        sync(device)
        start = time.time()

        for _ in range(times_to_run):
            pred = m(torch.from_numpy(a).to(device))

        sync(device)
        end = time.time()
        return end - start


iterations_per_run=50
print("run_seconds,batch_size,iterations_per_run,device")
for device in ['cpu', 'mps']:
    for batch_size in [1, 2, 4, 8, 16, 32, 64, 128]:
        a = np.float32(np.random.rand(batch_size, 3, 6, 7))

        # print("warm up  for 10 times for batch size:", batch_size, "device:", device)
        run(new_pt_m, batch_size, 10, device, a)

        print(f"{run(new_pt_m, batch_size, iterations_per_run, device, a):<5.3f},{batch_size:3},{iterations_per_run},{device}")

