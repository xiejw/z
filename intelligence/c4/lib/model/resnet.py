import torch

# builds a multi-layer Residual network.
def build_resnet_model(input_shape, num_classes, **kwargs):
    assert num_classes == 42
    assert input_shape == (6, 7, 3)

    m = ResNetModelWrapper()
    state_file_to_load = kwargs.get("state_file_to_load")
    if state_file_to_load:
        print("[sys] loading module state from path:", state_file_to_load)
        m.load_state_dict(
                torch.load(state_file_to_load, weights_only=True),
                strict=True)
    return m


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
        self.b0_add       =  ResNetModel.ResidualAddLayer()
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
        self.b1_add       =  ResNetModel.ResidualAddLayer()
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
        self.b2_add       = ResNetModel.ResidualAddLayer()
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
        self.b3_add       =  ResNetModel.ResidualAddLayer()
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


class ResNetModelWrapper(ResNetModel):

    def __init__(self):
        super(ResNetModelWrapper, self).__init__()

        # NOTE(2025-01-01): The mps on macOs is slower than cpu.
        # try:
        #     if torch.backends.mps.is_available():
        #         self.to('mps')
        # except:
        #     pass

        device = next(self.parameters()).device
        print(f"[sys] model is on {device}")
        self.device = device

    def predict(self, x):
        # This is the API client will call

        with torch.no_grad():
            self.eval()
            return self.__call__(torch.from_numpy(x).to(self.device))
