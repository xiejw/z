CMD              =  cmd
SRC              =  src
INC              =  include

EVE_FMT_FOLDERS  = ${CMD} ${SRC} ${INC}
EVE_PATH         = ../../y/ann/eve
include ${EVE_PATH}/eve.mk

CXXFLAGS        += -Iinclude
LDFLAGS         += ${EVE_LIB}

BINS            += ${BUILD}/sudoku_main
DEPS            += ${BUILD} ${BUILD_OBJ} ${EVE_LIB}

# ------------------------------------------------------------------------------
# Libs
#
MODULES          = ${BUILD_OBJ}/algos_dal.o

# ------------------------------------------------------------------------------
# Actions
#

.DEFAULT_GOAL   = compile

# incrementally compile all libraries.
compile: ${BINS} ${MODULES} ${DEPS}

run: compile
	${BUILD}/sudoku_main

release:
	@echo compile with RELEASE mode
	${MK} RELEASE=1 compile || exit 1

test: compile run

# ------------------------------------------------------------------------------
# Rules for sub-modules
#
${BUILD_OBJ}/algos_%.o: ${SRC}/algos/%.cc ${INC}/algos/*.h | ${DEPS}
	${CXX} -o $@ ${CXXFLAGS} -c $<

# ------------------------------------------------------------------------------
# Cmds
#
compile: ${BUILD}/sudoku_main

${BUILD}/sudoku_main: ${BUILD}/sudoku_main.o ${MODULES} | ${DEPS}
	${CXX} -o $@ $^ ${LDFLAGS}

${BUILD}/%_main.o: cmd/%/main.cc | ${DEPS}
	${CXX} ${CXXFLAGS} -o $@ -c $^

# ------------------------------------------------------------------------------
# Tests
#

TEST_LIBS       = ${BUILD_OBJ}/algos_dal_test.o
