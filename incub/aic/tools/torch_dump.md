This is the code snippet to dump torch tensor from a model

```
torch.set_grad_enabled(False)

params = []
print("==> tok_embeddings ", list(model.tok_embeddings.parameters()))
params.extend(list(model.tok_embeddings.parameters()))

# dump the tensor
def tensor_to_bytes(t):
    return t.to(dtype=torch.float32).numpy().tobytes()
def u32_to_bytes(i):
    # This assume mac m1.
    return i.to_bytes(4, byteorder='little', signed=False)

with open('/tmp/tensor_data.bin', 'wb') as f:
    # Header 1: Write total number of params
    f.write(u32_to_bytes(len(params)))

    # Header 2: Write all shapes
    for param in params:
        # Write dim count
        print("writing tensor with shape ", param.shape)
        f.write(u32_to_bytes(len(param.shape)))
        # Write shape
        [f.write(u32_to_bytes(x)) for x in param.shape]

    # Data: Write all values
    for param in params:
      f.write(tensor_to_bytes(param))
```
