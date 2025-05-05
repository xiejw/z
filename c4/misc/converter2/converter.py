# All shapes are recored first and then all weights.
import os

import torch

from model.resnet import build_resnet_model

torch.set_grad_enabled(False)
m = build_resnet_model(
        (6, 7, 3), 42, state_file_to_load=os.getenv("C4_STATE_FILE")).eval()

# print(m)
# print(m.conv2d.parameters())
# print(m.conv2d.weight)
# print(m.conv2d.bias)

layer = m.conv2d
params = [layer.weight, layer.bias]

for param in params:
    print("shape ", param.shape)
  print("value ", torch.reshape(param, (-1,))[:20])

def tensor_bo_bytes(t):
    return t.numpy().tobytes()
def u32_to_bytes(i):
    # This assume mac m1.
    return i.to_bytes(4, byteorder='little', signed=False)

with open('tensor_data.bin', 'wb') as f:
    # Write total number of params
    f.write(u32_to_bytes(len(params)))

    # Write all shapes
    for param in params:
        # Write dim count
        f.write(u32_to_bytes(len(param.shape)))
        # Write shape
        [f.write(u32_to_bytes(x)) for x in param.shape]

    # Write all values
    for param in params:
      f.write(tensor_bo_bytes(param))

