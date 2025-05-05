import torch
from model.resnet import build_resnet_model

torch.set_grad_enabled(False)
m = build_resnet_model((6, 7, 3), 42).eval()
# print(m)
# print(m.conv2d.parameters())
# print(m.conv2d.weight)
# print(m.conv2d.bias)

layer = m.conv2d
param = layer.weight
print("shape ", param.shape)
tensor_bytes = param.numpy().tobytes()
# Save to file
with open('tensor_data.bin', 'wb') as f:
    f.write(tensor_bytes)

