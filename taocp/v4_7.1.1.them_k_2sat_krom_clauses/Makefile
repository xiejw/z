MODS    += ${BUILD}/cmd_main.o
MODS    += ${BUILD_OBJS}/graph_sgb.o
MODS    += ${BUILD_OBJS}/two_sat_solver.o
MODS    += ${BUILD_OBJS}/log.o

include mk.tpl

# Few labels in the algorithm are not used.
CXXFLAGS += -Wno-unused-label

test: run
