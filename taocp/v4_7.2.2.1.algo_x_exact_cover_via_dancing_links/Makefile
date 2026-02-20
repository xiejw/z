MODS    += ${BUILD_OBJS}/log.o
MODS    += ${BUILD_OBJS}/dlink.o

MAIN_OUT = main

TEST_OUT = dlink_test

include mk.tpl

CXXFLAGS += -Wno-unused-label

$(eval $(call CMD_template,${TEST_OUT}))
$(eval $(call TEST_template,${TEST_OUT}))

