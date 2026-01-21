# Disable warning for multiple unsequencing access to the counter.
CXXFLAGS += -Wno-unsequenced

MODS    += ${BUILD}/cmd_main.o
MODS    += ${BUILD_OBJS}/log.o

include mk.tpl

test: run

timer:
	/usr/bin/time ${BUILD}/a.out
