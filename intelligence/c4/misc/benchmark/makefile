TORCH_DIR = ${HOME}/Workspace/build/torch/install

CFLAGS   += -I${TORCH_DIR}/include/
CFLAGS   += -I${TORCH_DIR}/include//torch/csrc/api/include

LDFLAGS  += -L${TORCH_DIR}/lib
LDFLAGS  += -Wl,-rpath,${TORCH_DIR}/lib
LDFLAGS  += -ltorch_cpu -ltorch -lc10

compile:
	clang++ -std=c++17 ${CFLAGS} ${LDFLAGS} 01_batch_size.cc

check:
	python -c 'import torch'

run_00_py:
	PYTHONPATH=../../lib python 00_batch_size.py

run_01_cc: compile
	./a.out ~/Desktop/traced_resnet_model.pt

clean:
	rm a.out
