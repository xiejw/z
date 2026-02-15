# Split the deps out as the test needs them.  main fn should be unique in each
# binary.
#
# TODO introduce mk.v2 to make main easier.
DEPS    += ${BUILD_OBJS}/log.o
DEPS    += ${BUILD_OBJS}/dlink.o

MODS    += ${DEPS}
MODS    += ${BUILD}/cmd_main.o

DLINK_TEST = ${BUILD}/dlink_test

include mk.tpl

compile: ${DLINK_TEST}

test: ${DLINK_TEST}
	${DLINK_TEST}

${DLINK_TEST}: ${DEPS} ${BUILD}/cmd_dlink_test.o | ${BUILD}
	${CXX} ${LDFLAGS} -o $@ $^

CXXFLAGS += -Wno-unused-private-field
