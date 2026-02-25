MODS    += ${BUILD_OBJS}/log.o
MODS    += ${BUILD_OBJS}/sat_watch.o

MAIN_OUT = main

TEST_OUT = dlink_test

include mk.tpl

CXXFLAGS += -Wno-unused-label

# Main is tested
$(eval $(call TEST_template,${MAIN_OUT}))

## TEST is tested
#$(eval $(call CMD_template,${TEST_OUT}))
#$(eval $(call TEST_template,${TEST_OUT}))

