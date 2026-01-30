# === Configurations -----------------------------------------------------------
#
DATA_FILE = https://github.com/xiejw/z/releases/download/v0.0.3/tensor_data.bin

# === Mods ---------------------------------------------------------------------
#
MODS    += ${BUILD}/cmd_main.o
MODS    += ${BUILD_OBJS}/game.o
MODS    += ${BUILD_OBJS}/log.o
MODS    += ${BUILD_OBJS}/mcts.o
MODS    += ${BUILD_OBJS}/nn.o
MODS    += ${BUILD_OBJS}/tensor.o

include mk.tpl

# === BLAS ---------------------------------------------------------------------
#
# Enable BLAS for macOs, which uses Accelerate framework.
ifeq ($(UNAME_S), Darwin)
CXXFLAGS += -DMACOS_ACCELERATE
CXXFLAGS += -DACCELERATE_NEW_LAPACK
LDFLAGS  += -framework Accelerate
endif

ifdef BLAS
CXXFLAGS += -DBLAS
LDFLAGS  += -lblas
endif

# === Knobs --------------------------------------------------------------------
#
# Control the iteration count for MCTS.
ifdef MCTS_ITER_CNT
CXXFLAGS += -DMCTS_ITER_CNT=${MCTS_ITER_CNT}
endif

# If define, the game will be played by two mcts-nn players.
ifdef MCTS_SELF_PLAY
CXXFLAGS += -DMCTS_SELF_PLAY=1
endif

# === Rules --------------------------------------------------------------------
#
run: compile ${BUILD}/tensor_data.bin

${BUILD}/tensor_data.bin: | ${BUILD}
	curl -L -C - -o $@ ${DATA_FILE} && sha256sum -c checksum.txt

test: compile

