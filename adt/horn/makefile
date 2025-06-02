CMD             =  cmd
SRC             =  src
INC             =  include

EVA_FMT_FOLDERS =  ${CMD} ${SRC} ${INC}
EVA_PATH        = ../../eva

include ${EVA_PATH}/eva.mk

# ------------------------------------------------------------------------------
# Configurations
#

CFLAGS          += -Iinclude -I${SRC}
LDFLAGS         += ${EVA_LIB}

ALL_LIBS       += ${BUILD_OBJ}/algos_horn.o

# ------------------------------------------------------------------------------
# Actions
#

.DEFAULT_GOAL   = compile

# Incrementally compile all libraries.
compile: ${ALL_LIBS} ${EVA_LIB}

release: horn

# ------------------------------------------------------------------------------
# rules for sub-modules
# ------------------------------------------------------------------------------
${BUILD_OBJ}/algos_%.o: ${SRC}/algos/%.c ${INC}/algos/*.h | ${BUILD_OBJ}
	${EVA_CC} -o $@ -c $<

# ------------------------------------------------------------------------------
# cmds
# ------------------------------------------------------------------------------

compile: $(patsubst ${CMD}/%/main.c,${BUILD}/%,$(wildcard ${CMD}/*/main.c))

$(eval $(call objs,horn,  $(BUILD), ${ALL_LIBS}))

# ------------------------------------------------------------------------------
# tests
# ------------------------------------------------------------------------------

# unit tests
TEST_LIBS       = ${BUILD_OBJ}/algos_horn_test.o

$(eval $(call objs,test,$(BUILD),$(ALL_LIBS) $(TEST_LIBS)))

