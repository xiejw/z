# vim: ft=python
#
# This script rewrites the original model with 4x4 kernel to 5x5 kernel to avoid
# extra input copy (as suggested by pytorch warning message).
#
# In addition, the Softmax has explicity dim set (as suggested by pytorch
# warning message as well).
#
# Appoarch wise, it looks like the following
#
# 1. Load all state dict with `strict=False`
# 2. Set the 5x5 kernel with zeros and copy the 4x4 kernel to right bottom part.
# 3. Check numerical results.


import os

from colorama import Fore
import numpy as np
import torch

torch.no_grad().__enter__()

FILE_TO_LOAD = os.path.expanduser("~/Desktop/c4-resnet.pt.state")
FILE_TO_DUMP = os.path.expanduser("~/Desktop/c4-resnet-5x5.pt.state")

#
# initialize the env
#

def print_bar():
    print("")
    print("========================")
    print("")

a = np.float32(np.random.rand(1, 3, 6, 7))

class ResNetModel4x4(torch.nn.Module):

    # a residual for ResNet
    class ResidualAddLayer(torch.nn.Module):
        def __init__(self):
            super(ResNetModel4x4.ResidualAddLayer, self).__init__()

        def forward(self, x, inp):
            return x + inp

    def __init__(self):
        super(ResNetModel4x4, self).__init__()

        self.conv2d = torch.nn.Conv2d(
                in_channels=3,
                out_channels=128,
                kernel_size=(4, 4),
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
                kernel_size=(4, 4),
                padding='same')
        self.b0_batch_n   =  torch.nn.BatchNorm2d(
                num_features=128,
                eps=0.001)
        self.b0_relu      =  torch.nn.ReLU(inplace=True)
        self.b0_conv2d_b  =  torch.nn.Conv2d(
                in_channels=128,
                out_channels=128,
                kernel_size=(4, 4),
                padding='same')
        self.b0_batch_n_b =  torch.nn.BatchNorm2d(
                num_features=128,
                eps=0.001)
        self.b0_add       =  ResNetModel4x4.ResidualAddLayer()
        self.b0_relu_b    =  torch.nn.ReLU(inplace=True)

        # block 1
        pos = pos + 7
        self.b1_conv2d    =  torch.nn.Conv2d(
                    in_channels=128,
                    out_channels=128,
                    kernel_size=(4, 4),
                    padding='same')
        self.b1_batch_n   =  torch.nn.BatchNorm2d(
                num_features=128,
                eps=0.001)
        self.b1_relu      =  torch.nn.ReLU(inplace=True)
        self.b1_conv2d_b  =  torch.nn.Conv2d(
                    in_channels=128,
                    out_channels=128,
                    kernel_size=(4, 4),
                    padding='same')
        self.b1_batch_n_b = torch.nn.BatchNorm2d(
                num_features=128,
                eps=0.001)
        self.b1_add       =  ResNetModel4x4.ResidualAddLayer()
        self.b1_relu_b    =  torch.nn.ReLU(inplace=True)

        # block 2
        pos = pos + 7
        self.b2_conv2d    = torch.nn.Conv2d(
                    in_channels=128,
                    out_channels=128,
                    kernel_size=(4, 4),
                    padding='same')
        self.b2_batch_n   =  torch.nn.BatchNorm2d(
                num_features=128,
                eps=0.001)
        self.b2_relu      =  torch.nn.ReLU(inplace=True)
        self.b2_conv2d_b  = torch.nn.Conv2d(
                    in_channels=128,
                    out_channels=128,
                    kernel_size=(4, 4),
                    padding='same')
        self.b2_batch_n_b = torch.nn.BatchNorm2d(
                num_features=128,
                eps=0.001)
        self.b2_add       = ResNetModel4x4.ResidualAddLayer()
        self.b2_relu_b    =  torch.nn.ReLU(inplace=True)

        # block 3
        pos = pos + 7
        self.b3_conv2d    =  torch.nn.Conv2d(
                    in_channels=128,
                    out_channels=128,
                    kernel_size=(4, 4),
                    padding='same')
        self.b3_batch_n   = torch.nn.BatchNorm2d(
                num_features=128,
                eps=0.001)
        self.b3_relu      =  torch.nn.ReLU(inplace=True)
        self.b3_conv2d_b  =  torch.nn.Conv2d(
                    in_channels=128,
                    out_channels=128,
                    kernel_size=(4, 4),
                    padding='same')
        self.b3_batch_n_b = torch.nn.BatchNorm2d(
                num_features=128,
                eps=0.001)
        self.b3_add       =  ResNetModel4x4.ResidualAddLayer()
        self.b3_relu_b    =  torch.nn.ReLU(inplace=True)

        # block 4
        pos = pos + 7
        self.b4_conv2d    =  torch.nn.Conv2d(
                    in_channels=128,
                    out_channels=128,
                    kernel_size=(4, 4),
                    padding='same')
        self.b4_batch_n   =  torch.nn.BatchNorm2d(
                num_features=128,
                eps=0.001)
        self.b4_relu      =  torch.nn.ReLU(inplace=True)
        self.b4_conv2d_b  =  torch.nn.Conv2d(
                    in_channels=128,
                    out_channels=128,
                    kernel_size=(4, 4),
                    padding='same')
        self.b4_batch_n_b = torch.nn.BatchNorm2d(
                num_features=128,
                eps=0.001)
        self.b4_add       = ResNetModel4x4.ResidualAddLayer()
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
        self.p_softmax   =  torch.nn.Softmax()

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
# do pytorch pred
#

pt_m = ResNetModel4x4().eval()
pt_m.load_state_dict(torch.load(FILE_TO_LOAD), strict=True)

print_bar()

pt_pred = pt_m(torch.from_numpy(a))
pt_pred, pt_v = pt_pred
print("( pt_m) (pred) ==>", pt_pred.numpy().flat[:10])
print("( pt_m) (pred) ==>", pt_pred.shape)

# print(Fore.GREEN  + "(pt_m) (pred trans) head ==>",
#         pt_pred.numpy().transpose(0, 2, 3, 1).flat[:10],
#         Fore.RESET)
# print(Fore.YELLOW + "(pt_m) (pred trans) tail ==>",
#         pt_pred.numpy().transpose(0, 2, 3, 1).flat[-10:],
#         Fore.RESET)

print(Fore.GREEN  + "(pt_m) (pred) head ==>",
        pt_pred.numpy().flat[:10],
        Fore.RESET)
print(Fore.YELLOW + "(pt_m) (pred) tail ==>",
        pt_pred.numpy().flat[-10:],
        Fore.RESET)
print(Fore.GREEN  + "(pt_m) (v)     ==>",
        pt_v.numpy(),
        Fore.RESET)

# ==============================================================================
#
# ==============================================================================


class ResNetModel5x5(torch.nn.Module):

    # a residual for ResNet
    class ResidualAddLayer(torch.nn.Module):
        def __init__(self):
            super(ResNetModel5x5.ResidualAddLayer, self).__init__()

        def forward(self, x, inp):
            return x + inp


    def rewrite_conv2d(self, state_dict):

        def rewrite(key_name, target_mm):
            w = state_dict.pop(key_name)

            print(f"  {key_name} shape src: {w.shape}")
            print(f"  {key_name} shape dst: {target_mm.weight.shape}")

            assert len(w.shape) == 4
            assert w.shape[2]   == 4
            assert w.shape[3]   == 4

            w_np = w.numpy()
            new_w_np = np.zeros([w.shape[0], w.shape[1], 5, 5], dtype=np.float32)
            new_w_np[:, :, 1, 1:5] = w_np[:, :, 0, :]
            new_w_np[:, :, 2, 1:5] = w_np[:, :, 1, :]
            new_w_np[:, :, 3, 1:5] = w_np[:, :, 2, :]
            new_w_np[:, :, 4, 1:5] = w_np[:, :, 3, :]
            target_mm.weight.copy_(torch.from_numpy(new_w_np))

        rewrite('conv2d.weight',      self.conv2d)
        rewrite('b0_conv2d.weight',   self.b0_conv2d)
        rewrite('b0_conv2d_b.weight', self.b0_conv2d_b)
        rewrite('b1_conv2d.weight',   self.b1_conv2d)
        rewrite('b1_conv2d_b.weight', self.b1_conv2d_b)
        rewrite('b2_conv2d.weight',   self.b2_conv2d)
        rewrite('b2_conv2d_b.weight', self.b2_conv2d_b)
        rewrite('b3_conv2d.weight',   self.b3_conv2d)
        rewrite('b3_conv2d_b.weight', self.b3_conv2d_b)
        rewrite('b4_conv2d.weight',   self.b4_conv2d)
        rewrite('b4_conv2d_b.weight', self.b4_conv2d_b)

        assert len(state_dict) == 0

        return self

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

sd = pt_m.state_dict()
# for name in sd:
#     print("  ", name)
keys_to_pop = ['conv2d.weight',
               'b0_conv2d.weight',
               'b0_conv2d_b.weight',
               'b1_conv2d.weight',
               'b1_conv2d_b.weight',
               'b2_conv2d.weight',
               'b2_conv2d_b.weight',
               'b3_conv2d.weight',
               'b3_conv2d_b.weight',
               'b4_conv2d.weight',
               'b4_conv2d_b.weight',
               ]
rewrite_sds = {}

for key_name in keys_to_pop:
    rewrite_sds[key_name] = sd.pop(key_name)

#sd.pop('conv2d.bias')
new_pt_m.load_state_dict(sd, strict=False)
new_pt_m.rewrite_conv2d(rewrite_sds)

print_bar()

del pt_m
new_pt_pred = new_pt_m(torch.from_numpy(a))
new_pt_pred, new_pt_v = new_pt_pred
print("( pt_m) (pred) ==>", new_pt_pred.numpy().flat[:10])
print("( pt_m) (pred) ==>", new_pt_pred.shape)

print(Fore.GREEN  + "(pt_m) (pred) head ==>",
        new_pt_pred.numpy().flat[:10],
        Fore.RESET)
print(Fore.YELLOW + "(pt_m) (pred) tail ==>",
        new_pt_pred.numpy().flat[-10:],
        Fore.RESET)
print(Fore.GREEN  + "(pt_m) (v)     ==>",
        new_pt_v.numpy(),
        Fore.RESET)

np.testing.assert_allclose(
        pt_pred.numpy(),
        new_pt_pred.numpy(),
        rtol=1e-03,
        atol=1e-03)

np.testing.assert_allclose(
        pt_v.numpy(),
        new_pt_v.numpy(),
        rtol=1e-03,
        atol=1e-03)

print_bar()
print('passed')

torch.save(new_pt_m.state_dict(), FILE_TO_DUMP)
print('saved')
