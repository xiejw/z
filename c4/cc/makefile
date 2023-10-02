# to install
#
#     pip install pybind11


UNAME := $(shell uname)

-include ../configure.mk

ifndef TORCH_DIR
$(error "TORCH_DIR is undefined. Run 'python configure.py' in top level folder")
endif


CXXFLAGS  += -O3 -Wall -std=c++17 -fPIC

ifndef C4_FILE_PATH
MODEL_CXXFLAGS   += -DC4_FILE_PATH='"${HOME}/Desktop/traced_resnet_model.pt"'
else
MODEL_CXXFLAGS   += -DC4_FILE_PATH='"${C4_FILE_PATH}"'
endif

ifdef C4_MODEL_ON_CPU
CXXFLAGS += -DC4_MODEL_ON_CPU=1
endif

TORCH_CXXFLAGS   += -I${TORCH_DIR}/include/
TORCH_CXXFLAGS   += -I${TORCH_DIR}/include/torch/csrc/api/include

PYBIND_CXXFLAGS  += $(shell python3 -m pybind11 --includes)
PY_EXT     = $(shell python3-config --extension-suffix)

# On macOS, we dont need torch libraries as python/torch has that already (in
# runtime)
#
# However, on Linux, we need.
ifeq ($(UNAME), Linux)
LDFLAGS  += -L${TORCH_DIR}/lib
LDFLAGS  += -Wl,-rpath,${TORCH_DIR}/lib
LDFLAGS  += -ltorch_cpu -ltorch -lc10
endif

LDFLAGS   += -shared

ifeq ($(UNAME), Darwin)
LDFLAGS   += -undefined dynamic_lookup
CXX        = clang++
else
# print(torch._C._GLIBCXX_USE_CXX11_ABI)
CXXFLAGS  += -D_GLIBCXX_USE_CXX11_ABI=0
CXX        = g++
endif

MODS       = model.o policy.o


compile: ${MODS}
	${CXX} ${CXXFLAGS} ${PYBIND_CXXFLAGS} \
		c4.cc ${MODS} ${LDFLAGS} -o xai_c4${PY_EXT}

model.o: model.cc model.h base.h
	${CXX} ${CXXFLAGS} ${MODEL_CXXFLAGS} ${TORCH_CXXFLAGS} \
		-I. -o model.o -c model.cc

policy.o: policy.cc policy.h base.h
	${CXX} ${CXXFLAGS} \
		-I. -o policy.o -c policy.cc

clean:
	rm -rf *`python3-config --extension-suffix` *.o


fmt:
	clang-format -i --style=file base.h
	clang-format -i --style=file model.cc
	clang-format -i --style=file model.h
	clang-format -i --style=file policy.h
	clang-format -i --style=file policy.cc
	clang-format -i --style=file c4.cc
