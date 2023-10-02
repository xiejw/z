
#
# configs from configure.py
#
-include ../configure.mk

ifndef C4_CONFIGURE_DONE
$(error "Project is not configured. Run 'python configure.py' in top level folder")
endif

#
# configs
#
UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
CXX               = clang++
else
CXX               = g++
endif

LDFLAGS          += -shared
CXXFLAGS         += -O3 -Wall -std=c++17 -fPIC

ifdef C4_MODEL_ON_CPU
CXXFLAGS         += -DC4_MODEL_ON_CPU=1
endif

MODEL_CXXFLAGS   += -DC4_FILE_PATH='"${C4_TRACED_MODEL_FILE}"'

TORCH_CXXFLAGS   += -I${TORCH_DIR}/include/
TORCH_CXXFLAGS   += -I${TORCH_DIR}/include/torch/csrc/api/include

# On macOS, we dont need torch libraries as python/torch has that already (in
# runtime)
#
# However, on Linux, we need.
ifeq ($(UNAME), Linux)
LDFLAGS          += -L${TORCH_DIR}/lib
LDFLAGS          += -Wl,-rpath,${TORCH_DIR}/lib
LDFLAGS          += -ltorch_cpu -ltorch -lc10
endif

#
# pybind11 flags
#
ifeq ($(UNAME), Darwin)
LDFLAGS          += -undefined dynamic_lookup
else
CXXFLAGS         += -D_GLIBCXX_USE_CXX11_ABI=${TORCH_CXX11_ABI}
endif

#
# actions
#
MODS              = model.o policy.o

compile: ${MODS}
	${CXX} ${CXXFLAGS} ${PYBIND_CXXFLAGS} \
		c4.cc ${MODS} ${LDFLAGS} -o xai_c4${PY_EXT_SUFFIX}

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
