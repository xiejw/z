#/usr/bin/env python3

import argparse
import os
import sys

from pathlib import Path
from string import Template

#
# flags
#
parser = argparse.ArgumentParser()
parser.add_argument("--model_dir",
                    help="specify where to find model weights")
args = parser.parse_args()

#
# constants
#
MK_FILE     = "configure.mk"
HOME        = str(Path.home())
C4_FILE_DIR = args.model_dir or os.path.join(HOME, "Desktop")

#
# check model weight files
#
def check_file_exists(f_path):
    if not os.path.isfile(f_path):
        print(f'[model file] failed to find the model file:', f_path)
        print(f'[model file] -> default search path is ~/Desktop.')
        print(f'[model file] -> try --model_dir flag to specify another dir.')
        sys.exit(1)
    return f_path

c4_state_file        = check_file_exists(
        os.path.join(C4_FILE_DIR, "c4-resnet-5x5.pt.state"))

#
# check python dependencies
#
try:
    import torch;
except ImportError:
    print("[python] package torch is required.")
    sys.exit(1)

try:
    import numpy;
except ImportError:
    print("[python] package numpy is required.")
    sys.exit(1)

#
# write configure into MK_FILE
#

makefile_tpl = Template(""
"C4_STATE_FILE          = $c4_state_file\n"
"C4_CONFIGURE_DONE      = done\n"
)

cfg = {
    "c4_state_file":        c4_state_file,
}

mk_str = makefile_tpl.substitute(cfg)
print(f"[sys] cfg is \n=== BEGIN ===\n{mk_str}=== END   ===")
print(f"[sys] writing to {MK_FILE}")
with open(MK_FILE, "w") as f:
    f.write(mk_str)

#
# bye
#
