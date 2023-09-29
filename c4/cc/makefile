# to install
#
#     pip install pybind11

TORCH_DIR = $(shell python -c 'import torch; print(torch.__path__[0])')

CXXFLAGS  += -O3 -Wall -std=c++17

MODEL_CXXFLAGS   += -DC4_FILE_PATH='"${HOME}/Desktop/traced_resnet_model.pt"'

TORCH_CXXFLAGS   += -I${TORCH_DIR}/include/
TORCH_CXXFLAGS   += -I${TORCH_DIR}/include/torch/csrc/api/include

PYBIND_CXXFLAGS  += $(shell python3 -m pybind11 --includes)
PY_EXT     = $(shell python3-config --extension-suffix)

# We dont need torch libraries as python/torch has that already (in runtime)
# LDFLAGS  += -L${TORCH_DIR}/lib
# LDFLAGS  += -Wl,-rpath,${TORCH_DIR}/lib
# LDFLAGS  += -ltorch_cpu -ltorch -lc10

LDFLAGS   += -fPIC -undefined dynamic_lookup -shared

MODS       = model.o policy.o


compile: ${MODS}
	clang++ ${CXXFLAGS} ${PYBIND_CXXFLAGS} \
		c4.cc ${MODS} ${LDFLAGS} -o xai_c4${PY_EXT}

model.o: model.cc model.h base.h
	clang++ ${CXXFLAGS} ${MODEL_CXXFLAGS} ${TORCH_CXXFLAGS} \
		-I. -o model.o -c model.cc

policy.o: policy.cc policy.h base.h
	clang++ ${CXXFLAGS} \
		-I. -o policy.o -c policy.cc

clean:
	rm *`python3-config --extension-suffix` *.o


fmt:
	clang-format -i --style=file base.h
	clang-format -i --style=file model.cc
	clang-format -i --style=file model.h
	clang-format -i --style=file policy.h
	clang-format -i --style=file policy.cc
	clang-format -i --style=file c4.cc
