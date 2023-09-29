# to install
#
#     pip install pybind11

TORCH_DIR = $(shell python -c 'import torch; print(torch.__path__[0])')

CXXFLAGS   += -DC4_FILE_PATH='"${HOME}/Desktop/traced_resnet_model.pt"'

CXXFLAGS   += -I${TORCH_DIR}/include/
CXXFLAGS   += -I${TORCH_DIR}/include//torch/csrc/api/include

# We dont need torch libraries as python/torch has that already (in runtime)
# LDFLAGS  += -L${TORCH_DIR}/lib
# LDFLAGS  += -Wl,-rpath,${TORCH_DIR}/lib
# LDFLAGS  += -ltorch_cpu -ltorch -lc10

CXXFLAGS  += -O3 -Wall -std=c++17
CXXFLAGS  += $(shell python3 -m pybind11 --includes)
LDFLAGS   += -fPIC -undefined dynamic_lookup -shared
PY_EXT     = $(shell python3-config --extension-suffix)

compile: model.o
	clang++ ${CXXFLAGS} c4.cc model.o ${LDFLAGS} -o xai_c4${PY_EXT}

model.o: model.cc model.h
	clang++ ${CXXFLAGS} -I. -o model.o -c model.cc

clean:
	rm *`python3-config --extension-suffix` *.o


fmt:
	clang-format -i --style=file model.cc
	clang-format -i --style=file model.h
	clang-format -i --style=file c4.cc
