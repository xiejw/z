#/usr/bin/env python3

import sys

from string import Template

MK_FILE = "configure.mk"

try:
    import torch;
except ImportError:
    print("torch is required. run 'make check' in top folder to check deps")
    sys.exit(1)

#
# write configure into MK_FILE
#

makefile_tpl = Template(""
"TORCH_DIR         = $torch_dir\n"
"C4_CONFIGURE_DONE = done\n"
)

cfg = {
    "torch_dir": torch.__path__[0],
}

mk_str = makefile_tpl.substitute(cfg)
print(f"[sys] cfg is \n=== BEGIN ===\n{mk_str}=== END   ===")
print(f"[sys] writing to {MK_FILE}")
with open(MK_FILE, "w") as f:
    f.write(mk_str)

#
# bye
#
