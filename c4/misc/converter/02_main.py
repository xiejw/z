import os
from pathlib import Path

import torch

from model import build_model

HOME_DIR = Path.home()
DUMP_DIR = os.path.join(HOME_DIR, 'Desktop')

m = build_model(
        (6, 7, 3),
        42,
        state_file_to_load=os.path.join(DUMP_DIR, "c4-resnet-5x5.pt.state"))

m.eval()
m.required_grad = False

# https://pytorch.org/tutorials/advanced/cpp_export.html
with torch.no_grad():
    sm = torch.jit.script(m)
    # print(sm.graph)
    saved_path = os.path.join(DUMP_DIR, "traced_resnet_model.pt")
    sm.save(saved_path)
    print(f'[sys] saved to {saved_path}')

# print test run
torch.manual_seed(123)
a = torch.randn((1, 3, 6, 7), dtype=torch.torch.float32)
with torch.no_grad():
    pred = m(a)

print(pred[0])
print(pred[1])

