# ------------------------------------------------------------------------------
# configs from configure.py
#
-include ../configure.mk

ifndef G5_CONFIGURE_DONE
$(error "Project is not configured. Run 'python configure.py' in top level dir")
endif

# ------------------------------------------------------------------------------
# configs
#

BUILD                  = .build

CXXFLAGS               = -I.
CXXFLAGS              += -O3 -Wall -std=c++20 -DNDEBUG

LDFLAGS               += -L${TORCH_DIR}/lib
LDFLAGS               += -Wl,-rpath,${TORCH_DIR}/lib
LDFLAGS               += -ltorch_cpu -ltorch -lc10

UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
CXX                    = clang++
else
CXX                    = g++
endif


ifeq ($(UNAME), Linux)
G5_MODEL_ON_CPU    = 1
endif

ifdef G5_MODEL_ON_CPU
CXXFLAGS         += -DG5_MODEL_ON_CPU=1
endif

MODEL_CXXFLAGS   += -DG5_FILE_PATH='"${G5_TRACED_MODEL_FILE}"'


# ------------------------------------------------------------------------------
# torch flags
#

TORCH_CXXFLAGS        += -I${TORCH_DIR}/include/
TORCH_CXXFLAGS        += -I${TORCH_DIR}/include/torch/csrc/api/include

ifeq ($(UNAME), Darwin)
else
CXXFLAGS              += -D_GLIBCXX_USE_CXX11_ABI=${TORCH_CXX11_ABI}
endif

# ------------------------------------------------------------------------------
# actions
#

MODS              = ${BUILD}/main.o ${BUILD}/model.o

run: compile
	${BUILD}/main

compile: ${BUILD}/main

${BUILD}:
	mkdir -p ${BUILD}

${BUILD}/main: ${MODS}
	${CXX} -o $@ ${MODS} ${LDFLAGS}

${BUILD}/main.o: main.cc model.h base.h | ${BUILD}
	${CXX} ${CXXFLAGS} ${TORCH_CXXFLAGS} -o $@ -c $<

${BUILD}/model.o: model.cc model.h base.h | ${BUILD}
	${CXX} ${CXXFLAGS} ${MODEL_CXXFLAGS} ${TORCH_CXXFLAGS} -o $@ -c $<


# ------------------------------------------------------------------------------
# house keeping
#

release: clean compile

test: compile

clean:
	rm -rf ${BUILD}

fmt:
	~/Workspace/y/tools/clang_format_all.sh .
