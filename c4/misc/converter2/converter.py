# All shapes are recored first and then all weights.
import os
import torch
from model.resnet import build_resnet_model

# === Helper Utils -------------------------------------------------------------

def display_tensor(t):
  print("  value (front) ", torch.reshape(t, (-1,))[:20])
  print("  value (tail ) ", torch.reshape(t, (-1,))[-20:])

def tensor_bo_bytes(t):
    return t.numpy().tobytes()
def u32_to_bytes(i):
    # This assume mac m1.
    return i.to_bytes(4, byteorder='little', signed=False)

# === Torch Configuration ------------------------------------------------------

torch.set_grad_enabled(False)
torch.manual_seed(123)

# === Model Resnet -------------------------------------------------------------
# Model can be found here
# https://github.com/xiejw/z/blob/main/c4/misc/converter/00_main.py#L280

m = build_resnet_model(
        (6, 7, 3), 42, state_file_to_load=os.getenv("C4_STATE_FILE")).eval()
# print(m)

inp = torch.randn((1, 3, 6, 7))

# print(list(m.conv2d.parameters()))
# print(m.conv2d.weight)
# print(m.conv2d.bias)

# Layer 0
layer = m.conv2d
out = layer(inp)
params = [inp, layer.weight, layer.bias]


# Layer 1
layer = m.batch_n
out = layer(out)
print("layer 1 eps", layer.eps)
params.extend([layer.weight, layer.bias, layer.running_mean, layer.running_var])

# Layer 2
layer = m.relu
out = layer(out)

params.append(out)

for param in params:
    print("shape ", param.shape)
    display_tensor(param)

with open('tensor_data.bin', 'wb') as f:
    # Header 1: Write total number of params
    f.write(u32_to_bytes(len(params)))

    # Header 2: Write all shapes
    for param in params:
        # Write dim count
        f.write(u32_to_bytes(len(param.shape)))
        # Write shape
        [f.write(u32_to_bytes(x)) for x in param.shape]

    # Data: Write all values
    for param in params:
      f.write(tensor_bo_bytes(param))

