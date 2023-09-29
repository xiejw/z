# vim: ft=python
#
# This code is used to translate ResNet-like Model for c4 from Keras to Pytorch.
#
# The strategy is as follows:
#
# 1. Create a ResNetConverter which takes all weights manually translated from
#    Keras.
# 2. Create a formal Pytorch ResNetModel in canonical way. Loading the weights
#    from ResNetConverter to it and then dump back (after numerical results
#    checking). With this, the code is usable by the RL game.


# disable tf and pytorch warnings
#
import util
util.suppress_tf_warning()
import warnings
warnings.filterwarnings("ignore")

#
# pip install torch  # if possible, install torch cpu
#
from colorama import Fore
from keras.models import Model
from keras.layers import Input, Dense, Flatten, Add, InputLayer
from keras.layers import Conv2D, BatchNormalization, ReLU
from keras import backend as K

import numpy as np
import torch


#
# configuration to change
#
LOAD_MODEL        = True
# LAYERS_TO_LOAD    = 3 + 7 * 5 + 5 + 3
# LAYERS_TO_LOAD    =  2
TOTAL_LAYER_COUNT = 50

# the final two heads are not easy to pop as the dead layer will be removed.
LAYERS_TO_POP     =  0
# ( TOTAL_LAYER_COUNT - LAYERS_TO_LOAD - 1) # need to load Input layer

#
# initialize the env
#

from game import GameConfig
from model import build_model
# from model import convert_inference_state_to_model_feature

config = GameConfig()
print(config)

# default path (in docker) to load weights
WEIGHTS_FILE = '/workdir/.build/weights.h5'

def print_bar():
    print("")
    print("=")
    print("==============>")
    print("=")

# Builds a model and loads weights.
def _build_model(config):
    num_classes = config.rows * config.columns
    input_shape = (config.rows, config.columns, 3)
    m = build_model(input_shape, num_classes)
    if LOAD_MODEL:
      print("Loading model from:", WEIGHTS_FILE)
      m.load_weights(WEIGHTS_FILE)
    return m

m        = _build_model(config)
# m_loaded = _build_model(config)

print_bar()
print("number of layers: ", len(m.layers))

m.summary()

print("{")
for l in m.layers:
    print(f"  {l.name}: {l}")
print("}")

# construct the layers to test
for i in range(LAYERS_TO_POP):
  m.layers.pop()

print_bar()
print("new keras model layers")
# new_m = Model(inputs=m.input, outputs=[m.layers[-2].output])
# new_m = Model(inputs=m.input, outputs=[m.layers[-1].output])
new_m = m

del m

print_bar()
print("number of layers (new_m): ", len( (new_m).layers))

print("{")
for l in new_m.layers:
    print(f"  {l.name}: {l}")
print("}")

# for idx, l in enumerate(new_m.layers):
#     l.set_weights(m_loaded.layers[idx].get_weights())

print_bar()

pt_layers = []

def print_weights(layer):
    for w in layer.weights:
        print(f"{Fore.CYAN} {w} {Fore.RESET}")

def convert_to_pytorch_layers(model):
    # sess = K.get_session()
    layers = model.layers
    for l in layers:
        if isinstance(l, InputLayer):
            print("")
            print("(convert) input layer:", l)
            print("                shape:", l.get_config()['batch_input_shape'])

        # conv2d
        elif isinstance(l, Conv2D):
            print("")
            print("(convert) conv2 layer:", l)
            print("          kernel_size:", l.kernel_size)
            print("                 bias:", l.use_bias)
            print("              padding:", l.padding)

            assert l.use_bias
            assert l.padding == 'same'
            # print_weights(l)
            assert len(l.weights) == 2

            ws = l.get_weights()

            kernel = l.weights[0]
            print("                kernel:", kernel)
            kernel_np = ws[0]
            print("     (var [:3]) kernel:", kernel_np.flat[:3])

            bias = l.weights[1]
            print("                  bias:", bias)
            bias_np = ws[1]
            print("       (var [:3]) bias:", bias_np[:3])

            py_l = torch.nn.Conv2d(
                    in_channels=kernel.shape[-2],
                    out_channels=kernel.shape[-1],
                    kernel_size=l.kernel_size,
                    padding='same')

            for name, p in py_l.named_parameters():
                print(f"             py weights: {name} {p.shape}")

            assert len(list(py_l.parameters())) == 2
            py_l.weight.copy_(torch.from_numpy(kernel_np.transpose(3, 2, 0, 1)).float())
            # print(py_l.weight.numpy().flat[0:3])
            py_l.bias.copy_(torch.from_numpy(bias_np).float())

            pt_layers.append(py_l)

        # batch norm
        elif isinstance(l, BatchNormalization):
            print("")
            print("(convert) batcN layer:", l)
            print("              epsilon:", l.epsilon)

            # print_weights(l)
            assert len(l.weights) == 4
            ws = l.get_weights()

            gamma = l.weights[0]
            gamma_np = ws[0]
            print("     (var [:3]) kernel:", gamma_np.flat[:3])
            beta = l.weights[1]
            beta_np = ws[1]
            print("       (var [:3]) bias:", beta_np.flat[:3])

            print("                gamma:", gamma)
            print("                 bias:", bias)

            py_l = torch.nn.BatchNorm2d(
                    num_features=gamma.shape[0],
                    eps=l.epsilon)

            for name, p in py_l.named_parameters():
                print(f"             py weights: {name} {p.shape}")

            # for k, v in py_l.state_dict().items():
            #     print(f"             py state: {k} {v}")

            assert len(list(py_l.parameters())) == 2
            py_l.weight.copy_(torch.from_numpy(gamma_np).float())
            py_l.bias.copy_(torch.from_numpy(beta_np).float())
            py_l.running_mean.copy_(torch.from_numpy(ws[2]).float())
            py_l.running_var.copy_(torch.from_numpy(ws[3]).float())

            pt_layers.append(py_l)

        # relu
        elif isinstance(l, ReLU):

            py_l = torch.nn.ReLU(inplace=True)

            pt_layers.append(py_l)

        # add
        elif isinstance(l, Add):
            py_l = ResNetConverter.ResidualAddLayer()
            pt_layers.append(py_l)

        # flattern
        elif isinstance(l, Flatten):
            py_l = torch.nn.Flatten()
            pt_layers.append(py_l)

        # dense
        elif isinstance(l, Dense):
            actn = l.get_config()['activation']
            print("")
            print("(convert) dense layer:", l)
            print("         kernel_shape:", l.kernel.shape)
            print("           bias_shape:", l.bias.shape)
            print("           activation:", actn)

            print_weights(l)
            assert len(l.weights) == 2

            ws = l.get_weights()

            py_l = torch.nn.Linear(
                    in_features=l.kernel.shape[0],
                    out_features=l.kernel.shape[1])

            for name, p in py_l.named_parameters():
                print(f"             py weights: {name} {p.shape}")

            # torch is <out, in> shape
            py_l.weight.copy_(torch.from_numpy(ws[0].transpose(1, 0)).float())
            py_l.bias.copy_(torch.from_numpy(ws[1]).float())
            pt_layers.append(py_l)

            if actn == 'softmax':
                print("")
                print("(convert) added softmax due to actn for:", l)
                pt_layers.append(torch.nn.Softmax())
            elif actn == 'relu':
                print("")
                print("(convert) added relu due to actn for:", l)
                pt_layers.append(torch.nn.ReLU())
            elif actn == 'tanh':
                print("")
                print("(convert) added tanh due to actn for:", l)
                pt_layers.append(torch.nn.Tanh())
            else:
                raise Exception("unknown actn " + actn)

        else:
            raise Exception("unimpl layer" + str(l))


class ResNetConverter(torch.nn.Module):

    # a residual for ResNet
    class ResidualAddLayer(torch.nn.Module):
        def __init__(self):
            super(ResNetConverter.ResidualAddLayer, self).__init__()

        def forward(self, x, inp):
            return x + inp

    def __init__(self, layers):
        super(ResNetConverter, self).__init__()

        self.conv2d  = layers[0]
        self.batch_n = layers[1]
        self.relu    = layers[2]

        # block 0
        pos = 3
        self.b0_conv2d    =  layers[pos + 0]
        self.b0_batch_n   =  layers[pos + 1]
        self.b0_relu      =  layers[pos + 2]
        self.b0_conv2d_b  =  layers[pos + 3]
        self.b0_batch_n_b =  layers[pos + 4]
        self.b0_add       =  layers[pos + 5]
        self.b0_relu_b    =  layers[pos + 6]

        # block 1
        pos = pos + 7
        self.b1_conv2d    =  layers[pos + 0]
        self.b1_batch_n   =  layers[pos + 1]
        self.b1_relu      =  layers[pos + 2]
        self.b1_conv2d_b  =  layers[pos + 3]
        self.b1_batch_n_b =  layers[pos + 4]
        self.b1_add       =  layers[pos + 5]
        self.b1_relu_b    =  layers[pos + 6]

        # block 2
        pos = pos + 7
        self.b2_conv2d    =  layers[pos + 0]
        self.b2_batch_n   =  layers[pos + 1]
        self.b2_relu      =  layers[pos + 2]
        self.b2_conv2d_b  =  layers[pos + 3]
        self.b2_batch_n_b =  layers[pos + 4]
        self.b2_add       =  layers[pos + 5]
        self.b2_relu_b    =  layers[pos + 6]

        # block 3
        pos = pos + 7
        self.b3_conv2d    =  layers[pos + 0]
        self.b3_batch_n   =  layers[pos + 1]
        self.b3_relu      =  layers[pos + 2]
        self.b3_conv2d_b  =  layers[pos + 3]
        self.b3_batch_n_b =  layers[pos + 4]
        self.b3_add       =  layers[pos + 5]
        self.b3_relu_b    =  layers[pos + 6]

        # block 4
        pos = pos + 7
        self.b4_conv2d    =  layers[pos + 0]
        self.b4_batch_n   =  layers[pos + 1]
        self.b4_relu      =  layers[pos + 2]
        self.b4_conv2d_b  =  layers[pos + 3]
        self.b4_batch_n_b =  layers[pos + 4]
        self.b4_add       =  layers[pos + 5]
        self.b4_relu_b    =  layers[pos + 6]

        # assert (pos + 7) == len(layers)

        # policy head

        pos = pos + 7
        self.p_conv2d    =  layers[pos + 1]
        self.p_batch_n   =  layers[pos + 3]
        self.p_relu      =  layers[pos + 5]
        self.p_flatten   =  layers[pos + 7]
        self.p_dense     =  layers[pos + 10]
        self.p_softmax   =  layers[pos + 11]

        # value head

        self.v_conv2d    =  layers[pos + 0]
        self.v_batch_n   =  layers[pos + 2]
        self.v_relu      =  layers[pos + 4]
        self.v_flatten   =  layers[pos + 6]
        self.v_dense_1   =  layers[pos + 8]
        self.v_relu      =  layers[pos + 9]
        self.v_dense_2   =  layers[pos + 12]
        self.v_tanh      =  layers[pos + 13]

    def rewrite_policy_dense(self):
        "rewrite the policy tower to avoid transpose"

        rows, cols = config.rows, config.columns
        in_dim = rows * cols
        out_dim = int(self.p_conv2d.out_channels)

        weight = self.p_dense.weight
        print_bar()
        print(f"  rewrite policy dense weight: {weight.shape}")
        print(f"               conv2d out dim: {out_dim}")
        print(f"               conv2d in  dim: {in_dim}")
        assert weight.shape[1] == in_dim * out_dim

        # transposed in pytorch dense
        # the original form ia nhwc we need to translate to nchw
        weight_np = weight.numpy()
        new_weight_np = np.zeros(weight_np.shape, dtype=weight_np.dtype)

        for o in range(out_dim):
            for i in range(in_dim):
                idx_in_nchw = o * in_dim + i
                idx_in_nhwc = i * out_dim + o

                new_weight_np[:, idx_in_nchw] = weight_np[:, idx_in_nhwc]

        # print(new_weight_np)
        for row in range(int(weight.shape[0])):
            message = (f"new {new_weight_np[row,:].sum()} " +
                    f"old {weight_np[row,:].sum()} " +
                    f"index {row}")
            assert abs(new_weight_np[row, :].sum() -
                    np.array(weight_np[row, :].sum())) < 1e-4, message

        self.p_dense.weight.copy_(torch.from_numpy(new_weight_np).float())
        return self

    def rewrite_value_dense(self):
        "rewrite the value tower to avoid transpose"

        rows, cols = config.rows, config.columns
        in_dim = rows * cols
        out_dim = int(self.v_conv2d.out_channels)

        weight = self.v_dense_1.weight
        print_bar()
        print(f"  rewrite value  dense weight: {weight.shape}")
        print(f"               conv2d out dim: {out_dim}")
        print(f"               conv2d in  dim: {in_dim}")
        assert weight.shape[1] == in_dim * out_dim

        # transposed in pytorch dense
        # the original form ia nhwc we need to translate to nchw
        weight_np = weight.numpy()
        new_weight_np = np.zeros(weight_np.shape, dtype=weight_np.dtype)

        for o in range(out_dim):
            for i in range(in_dim):
                idx_in_nchw = o * in_dim + i
                idx_in_nhwc = i * out_dim + o

                new_weight_np[:, idx_in_nchw] = weight_np[:, idx_in_nhwc]

        # print(new_weight_np)
        for row in range(int(weight.shape[0])):
            message = (f"new {new_weight_np[row,:].sum()} " +
                    f"old {weight_np[row,:].sum()} " +
                    f"index {row}")
            assert abs(new_weight_np[row, :].sum() -
                    np.array(weight_np[row, :].sum())) < 1e-4, message

        self.v_dense_1.weight.copy_(torch.from_numpy(new_weight_np).float())
        return self

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
        # TODO: note tranpose is NOT need as the weights got translated
        # x = torch.permute(x, (0, 2, 3, 1))
        x = self.p_flatten  (x)
        x = self.p_dense (x)
        x = self.p_softmax (x)

        p = x

        # value head
        x = self.v_conv2d    (inp)
        x = self.v_batch_n   (x)
        x = self.v_relu      (x)
        # TODO: note tranpose is NOT need as the weights got translated
        # x = torch.permute(x, (0, 2, 3, 1))
        x = self.v_flatten   (x)
        x = self.v_dense_1   (x)
        x = self.v_relu      (x)
        x = self.v_dense_2   (x)
        x = self.v_tanh      (x)

        v = x

        return (p, v)

class ResNetModel(torch.nn.Module):

    # a residual for ResNet
    class ResidualAddLayer(torch.nn.Module):
        def __init__(self):
            super(ResNetModel.ResidualAddLayer, self).__init__()

        def forward(self, x, inp):
            return x + inp

    def __init__(self):
        super(ResNetModel, self).__init__()

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
        self.b0_add       =  ResNetModel.ResidualAddLayer()
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
        self.b1_add       =  ResNetModel.ResidualAddLayer()
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
        self.b2_add       = ResNetModel.ResidualAddLayer()
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
        self.b3_add       =  ResNetModel.ResidualAddLayer()
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
        self.b4_add       = ResNetModel.ResidualAddLayer()
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

with torch.no_grad():
  convert_to_pytorch_layers(new_m)
  pt_m = (ResNetConverter(pt_layers)
          .eval()
          .rewrite_policy_dense()
          .rewrite_value_dense())

a = np.float32(np.random.rand(1, config.rows, config.columns, 3))

#
# do keras pred
#
print_bar()
keras_pred, keras_v = new_m.predict(a)
print(Fore.GREEN  + "(new_m) (pred) head ==>", keras_pred.flat[:10], Fore.RESET)
print(Fore.YELLOW + "(new_m) (pred) tail ==>", keras_pred.flat[-10:], Fore.RESET)
print("(new_m) (pred) ==>", keras_pred.shape)

print(Fore.GREEN + "(new_m) (value) ==>", keras_v, Fore.RESET)

#
# do pytorch pred
#
print_bar()
with torch.no_grad():
  pt_pred = pt_m(torch.from_numpy(a.transpose(0, 3, 1, 2)))
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

np.testing.assert_allclose(
        keras_pred,
        pt_pred.numpy(),
        rtol=1e-03,
        atol=1e-03)

np.testing.assert_allclose(
        keras_v,
        pt_v.numpy(),
        rtol=1e-03,
        atol=1e-03)
print_bar()
print('passed')


#
# do pytorch pred from new model with static config
#
new_pt_m = ResNetModel().eval()
new_pt_m.load_state_dict(pt_m.state_dict())

## uncomment to debug
# print_bar()
# print("pt_m")
# print("{")
# for name in pt_m.state_dict():
#     print("  ", name)
#
# print("}")
#
# print("new_pt_m")
# print("{")
# for name in new_pt_m.state_dict():
#     print("  ", name)
#
# print("}")

del pt_m
del pt_pred
del pt_v

print_bar()
with torch.no_grad():
  new_pt_pred = new_pt_m(torch.from_numpy(a.transpose(0, 3, 1, 2)))
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
        keras_pred,
        new_pt_pred.numpy(),
        rtol=1e-03,
        atol=1e-03)

np.testing.assert_allclose(
        keras_v,
        new_pt_v.numpy(),
        rtol=1e-03,
        atol=1e-03)

print_bar()
print('passed 2')

torch.save(new_pt_m.state_dict(), "resnet.pt.state")
