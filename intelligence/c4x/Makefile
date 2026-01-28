# === Configurations -----------------------------------------------------------
#
UNAME     = $(shell uname)
BUILD     = .build
DATA_FILE = https://github.com/xiejw/z/releases/download/v0.0.3/tensor_data.bin

# === CFLAGS and LDFLAGS -------------------------------------------------------
#
CXXFLAGS += -std=c++17 -Wall -Werror -pedantic -Wextra -Wfatal-errors -Wconversion
LDFLAGS  += -lm

ifdef RELEASE
CXXFLAGS   += -DNDEBUG -O3 -march=native
LDFLAGS  += -ffast-math
else
CXXFLAGS   += -g
endif

ifdef ASAN
LDFLAGS  += -fsanitize=address
endif

# Enable BLAS for macOs, which uses Accelerate framework.
ifeq ($(UNAME), Darwin)
CXXFLAGS   += -DMACOS_ACCELERATE
CXXFLAGS   += -DACCELERATE_NEW_LAPACK
LDFLAGS  += -framework Accelerate
endif

ifdef BLAS
CXXFLAGS   += -DBLAS
LDFLAGS  += -lblas
endif

# === Knobs --------------------------------------------------------------------
#
# Control the iteration count for MCTS.
ifdef MCTS_ITER_CNT
CXXFLAGS   += -DMCTS_ITER_CNT=${MCTS_ITER_CNT}
endif

# If define, the game will be played by two mcts-nn players.
ifdef MCTS_SELF_PLAY
CXXFLAGS   += -DMCTS_SELF_PLAY=1
endif


# === Rules --------------------------------------------------------------------
#
run: compile ${BUILD}/tensor_data.bin | ${BUILD}
	./${BUILD}/a.out

compile: | ${BUILD}
	${CXX} -o ${BUILD}/a.out ${CXXFLAGS} ${LDFLAGS} main.cc

${BUILD}/tensor_data.bin: | ${BUILD}
	curl -L -C - -o $@ ${DATA_FILE} && sha256sum -c checksum.txt

${BUILD}:
	mkdir -p ${BUILD}

test: compile

fmt:
	~/Workspace/y/tools/clang_format_all.sh .

clean:
	rm -rf ${BUILD}
