#
# configs from configure.py
#
-include configure.mk

ifndef C4_CONFIGURE_DONE
$(error "Project is not configured. Run 'python configure.py' in top level folder")
endif

BUILD                  = .build
#
# configs
#
UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
CXX               = clang++
else
CXX               = g++
endif

CXXFLAGS          = -I.
CXXFLAGS         += -O3 -Wall -std=c++20 -DNDEBUG

ifeq ($(UNAME), Linux)
C4_MODEL_ON_CPU    = 1
endif

ifdef C4_MODEL_ON_CPU
CXXFLAGS         += -DC4_MODEL_ON_CPU=1
endif

MODEL_CXXFLAGS   += -DC4_FILE_PATH='"${C4_TRACED_MODEL_FILE}"'

TORCH_CXXFLAGS   += -I${TORCH_DIR}/include/
TORCH_CXXFLAGS   += -I${TORCH_DIR}/include/torch/csrc/api/include

LDFLAGS          += -L${TORCH_DIR}/lib
LDFLAGS          += -Wl,-rpath,${TORCH_DIR}/lib
LDFLAGS          += -ltorch_cpu -ltorch -lc10

#
# Torch flags
#
ifeq ($(UNAME), Darwin)
else
CXXFLAGS         += -D_GLIBCXX_USE_CXX11_ABI=${TORCH_CXX11_ABI}
endif

#
# actions
#
MODS              = ${BUILD}/main.o ${BUILD}/model.o

compile: ${MODS} ${BUILD}/main

${BUILD}/main: ${MODS}
	${CXX} -o $@ ${LDFLAGS} ${MODS}

${BUILD}/main.o: main.cc model.h base.h | ${BUILD}
	${CXX} ${CXXFLAGS} ${TORCH_CXXFLAGS} -o $@ -c main.cc

${BUILD}/model.o: model.cc model.h base.h | ${BUILD}
	${CXX} ${CXXFLAGS} ${MODEL_CXXFLAGS} ${TORCH_CXXFLAGS} -o $@ -c model.cc

${BUILD}:
	mkdir -p ${BUILD}

clean:
	rm -rf ${BUILD}

fmt:
	~/Workspace/y/tools/clang_format_all.sh .
