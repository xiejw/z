#/usr/bin/env python3

import os
import platform
import subprocess
import sys

from distutils.spawn import find_executable
from pathlib import Path
from string import Template

MK_FILE     = "configure.mk"
HOME        = str(Path.home())
C4_FILE_DIR = os.path.join(HOME, "Desktop")
UNAME       = platform.system()

#
# check python dependencies
#
try:
    import torch;
except ImportError:
    print("[python] package torch is required.")
    sys.exit(1)

if UNAME == 'Darwin':
    assert torch.backends.mps.is_available(), "macOS requires mps in torch"

try:
    import numpy;
except ImportError:
    print("[python] package numpy is required.")
    sys.exit(1)

try:
    import pybind11;
except ImportError:
    print("[python] package pybind11 is required.")
    sys.exit(1)

#
# check system configs
#

# find compiler
if UNAME == 'Darwin':
    assert find_executable('clang++'), "clang++ is required on macOS"
else:
    assert find_executable('g++'), "clang++ is required on Linux"

# next line only matters for Linux
torch_cxx11_abi = 1 if torch._C._GLIBCXX_USE_CXX11_ABI else 0

def get_cmd_output(cmd_w_args):
    return str(subprocess.check_output(cmd_w_args).decode('utf-8').strip())

py_ext_suffix = get_cmd_output(['python3-config', '--extension-suffix'])
pybind_cxxflags = get_cmd_output(['python3', '-m', 'pybind11', '--includes'])

#
# write configure into MK_FILE
#

makefile_tpl = Template(""
"PY_EXT_SUFFIX          = $py_ext_suffix\n"
"PYBIND_CXXFLAGS        = $pybind_cxxflags\n"
"TORCH_DIR              = $torch_dir\n"
"TORCH_CXX11_ABI        = $torch_cxx11_abi\n"
"C4_STATE_FILE          = $c4_state_file\n"
"C4_TRACED_MODEL_FILE   = $c4_traced_model_file\n"
"C4_CONFIGURE_DONE      = done\n"
)

cfg = {
    "py_ext_suffix":        py_ext_suffix,
    "pybind_cxxflags":      pybind_cxxflags,
    "torch_dir":            torch.__path__[0],
    "torch_cxx11_abi":      torch_cxx11_abi,
    "c4_state_file":        os.path.join(C4_FILE_DIR, "c4-resnet-5x5.pt.state"),
    "c4_traced_model_file": os.path.join(C4_FILE_DIR, "traced_resnet_model.pt"),
}

mk_str = makefile_tpl.substitute(cfg)
print(f"[sys] cfg is \n=== BEGIN ===\n{mk_str}=== END   ===")
print(f"[sys] writing to {MK_FILE}")
with open(MK_FILE, "w") as f:
    f.write(mk_str)

#
# bye
#
