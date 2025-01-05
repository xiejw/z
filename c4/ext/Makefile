FMT             = ~/Workspace/y/tools/clang_format_all.sh

fmt:
	${FMT} .

.DEFAULT_GOAL = compile
#
# configs from configure.py
#
-include ../configure.mk

ifndef C4_CONFIGURE_DONE
$(error "Project is not configured. Run './run.sh' in top level folder")
endif

#
# configs
#
UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
CXX               = clang++
else
CXX               = clang++
endif

EXT_LD_FLAGS     += -shared
CXXFLAGS         += -O3 -std=gnu++23 -fPIC
CXXFLAGS         += -Wall -Werror -pedantic -Wextra -Wfatal-errors -Wconversion

ifeq ($(UNAME), Linux)
MODEL_ON_CPU    = 1
endif

ifdef MODEL_ON_CPU
CXXFLAGS         += -DMODEL_ON_CPU=1
endif

CXXFLAGS   += -DC4_FILE_PATH='"${C4_TRACED_MODEL_FILE}"'

CXXFLAGS   += -isystem${TORCH_DIR}/include/
CXXFLAGS   += -isystem${TORCH_DIR}/include/torch/csrc/api/include

# On macOS, we dont need torch libraries as python/torch has that already (in
# runtime)
#
# However, on Linux, we need.
ifeq ($(UNAME), Linux)
LDFLAGS          += -L${TORCH_DIR}/lib
LDFLAGS          += -Wl,-rpath,${TORCH_DIR}/lib
LDFLAGS          += -ltorch_cpu -ltorch -lc10
LDFLAGS          += -fuse-ld=lld
endif

ifeq ($(UNAME), Darwin)
else
CXXFLAGS         += -D_GLIBCXX_USE_CXX11_ABI=${TORCH_CXX11_ABI}
endif

#
# actions
#
BUILD = .build

EXT_MODS = ${BUILD}/model.o
MODS     = ${BUILD}/main.o ${EXT_MODS}

${BUILD}/%.o: %.cc
	${CXX} ${CXXFLAGS} -c $< -o $@

main: ${MODS} | ${BUILD}
	${CXX} ${LDFLAGS} $^ -o ${BUILD}/main

compile: ${EXT_MODS} | ${BUILD}
	${CXX} ${EXT_LD_FLAGS} ${LDFLAGS} $^ -o ${BUILD}/c4_sys.so


${BUILD}:
	mkdir -p ${BUILD}
