TORCH_DIR = ${HOME}/Workspace/build/torch/install

CFLAGS   += -I${TORCH_DIR}/include/
CFLAGS   += -I${TORCH_DIR}/include//torch/csrc/api/include

LDFLAGS  += -L${TORCH_DIR}/lib
LDFLAGS  += -Wl,-rpath,${TORCH_DIR}/lib
LDFLAGS  += -ltorch_cpu -ltorch -lc10

compile:
	clang++ -std=c++17 ${CFLAGS} ${LDFLAGS} 02_main.cc

check:
	python -c 'import torch'

run_02_cc: compile
	./a.out ~/Desktop/traced_resnet_model.pt

run_02_py:
	PYTHONPATH=../../lib python 02_main.py

clean:
	rm a.out
