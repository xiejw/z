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

# Generate random inputs and save it for comparision.
inp = torch.randn((1, 3, 6, 7))
params = [inp]

# === Model Resnet -------------------------------------------------------------
# Model can be found here
#
# Ref 1: https://github.com/xiejw/z/blob/main/c4/misc/converter/00_main.py#L280
# Ref 2: https://github.com/xiejw/z/blob/main/c4/lib/model/resnet.py#L29

m = build_resnet_model(
        (6, 7, 3), 42, state_file_to_load=os.getenv("C4_STATE_FILE")).eval()
# print(m)

model_policy_out = m(inp)[0]

# === Prepare to decompose all layers and do computation on each layer ---------

# Layer 0
layer = m.conv2d
out = layer(inp)
params.extend([layer.weight, layer.bias])

# Layer 1
layer = m.batch_n
out = layer(out)
# print("layer 1 eps", layer.eps)
params.extend([layer.weight, layer.bias, layer.running_mean, layer.running_var])

# Layer 2
layer = m.relu
out = layer(out)

# # Block 0
dup = out
layer = m.b0_conv2d
out = layer(out)
params.extend([layer.weight, layer.bias])

layer = m.b0_batch_n
out = layer(out)
params.extend([layer.weight, layer.bias, layer.running_mean, layer.running_var])

layer = m.relu
out = layer(out)

layer = m.b0_conv2d_b
out = layer(out)
params.extend([layer.weight, layer.bias])

layer = m.b0_batch_n_b
out = layer(out)
params.extend([layer.weight, layer.bias, layer.running_mean, layer.running_var])

out = m.b0_relu_b(m.b0_add(out, dup))
del dup

# # Block 1
dup = out
layer = m.b1_conv2d
out = layer(out)
params.extend([layer.weight, layer.bias])

layer = m.b1_batch_n
out = layer(out)
params.extend([layer.weight, layer.bias, layer.running_mean, layer.running_var])

layer = m.relu
out = layer(out)

layer = m.b1_conv2d_b
out = layer(out)
params.extend([layer.weight, layer.bias])

layer = m.b1_batch_n_b
out = layer(out)
params.extend([layer.weight, layer.bias, layer.running_mean, layer.running_var])

out = m.b1_relu_b(m.b1_add(out, dup))
del dup

# # Block 2
dup = out
layer = m.b2_conv2d
out = layer(out)
params.extend([layer.weight, layer.bias])

layer = m.b2_batch_n
out = layer(out)
params.extend([layer.weight, layer.bias, layer.running_mean, layer.running_var])

layer = m.relu
out = layer(out)

layer = m.b2_conv2d_b
out = layer(out)
params.extend([layer.weight, layer.bias])

layer = m.b2_batch_n_b
out = layer(out)
params.extend([layer.weight, layer.bias, layer.running_mean, layer.running_var])

out = m.b2_relu_b(m.b2_add(out, dup))
del dup

# # Block 3
dup = out
layer = m.b3_conv2d
out = layer(out)
params.extend([layer.weight, layer.bias])

layer = m.b3_batch_n
out = layer(out)
params.extend([layer.weight, layer.bias, layer.running_mean, layer.running_var])

layer = m.relu
out = layer(out)

layer = m.b3_conv2d_b
out = layer(out)
params.extend([layer.weight, layer.bias])

layer = m.b3_batch_n_b
out = layer(out)
params.extend([layer.weight, layer.bias, layer.running_mean, layer.running_var])

out = m.b3_relu_b(m.b3_add(out, dup))
del dup

# # Block 4
dup = out
layer = m.b4_conv2d
out = layer(out)
params.extend([layer.weight, layer.bias])

layer = m.b4_batch_n
out = layer(out)
params.extend([layer.weight, layer.bias, layer.running_mean, layer.running_var])

layer = m.relu
out = layer(out)

layer = m.b4_conv2d_b
out = layer(out)
params.extend([layer.weight, layer.bias])

layer = m.b4_batch_n_b
out = layer(out)
params.extend([layer.weight, layer.bias, layer.running_mean, layer.running_var])

out = m.b4_relu_b(m.b4_add(out, dup))
del dup

# Policy Head
dup = out
layer = m.p_conv2d
out = layer(out)
params.extend([layer.weight, layer.bias])

layer = m.p_batch_n
out = layer(out)
params.extend([layer.weight, layer.bias, layer.running_mean, layer.running_var])

layer = m.relu
out = layer(out)

out = m.p_flatten(out)

layer = m.p_dense
out = layer(out)
params.extend([layer.weight, layer.bias])

out = m.p_softmax(out)

# We saved two outputs for comparison.
# First is the layer by layer computation result.
# Second is the model output directly. They should be same.
params.append(out)
params.append(model_policy_out)

# === Save all tensors to output file ------------------------------------------

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

