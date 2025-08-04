CMD              =  cmd
SRC              =  src
INC              =  include

EVE_FMT_FOLDERS  = ${SRC}
EVE_PATH         = ../../y/ann/eve
include ${EVE_PATH}/eve.mk

ZION_PATH        = ../../y/ann/zion/cc
CXXFLAGS        += -I${ZION_PATH}/include

CXXFLAGS        += -Iinclude
LDFLAGS         += ${EVE_LIB}

DEPS            += ${BUILD} ${BUILD_OBJ} ${EVE_LIB}

# ------------------------------------------------------------------------------
# Mods
#
MODS            += ${BUILD_OBJ}/sat_solver.o
MODS            += ${BUILD_OBJ}/sat_watch.o

# TEST_MODS       += ${BUILD_OBJ}/algos_sat_watch_test.o
# TEST_MODS       += ${BUILD_OBJ}/algos_sat_watch_dep_test.o

# ------------------------------------------------------------------------------
# Actions
#

.DEFAULT_GOAL   = run

# incrementally compile all libraries.
compile: ${MODS} ${DEPS}

run: compile
	${BUILD}/sat_watch_main

release:
	@echo compile with RELEASE mode
	${MK} RELEASE=1 compile run || exit 1


test: compile
#	${BUILD}/test_main

# ------------------------------------------------------------------------------
# Rules for sub-modules
#
${BUILD_OBJ}/%.o: ${SRC}/%.cc ${SRC}/*.h | ${DEPS}
	${CXX} -o $@ ${CXXFLAGS} -c $<

# ------------------------------------------------------------------------------
# Cmds
#
compile: ${BUILD}/sat_watch_main  # ${BUILD}/test_main

${BUILD}/sat_watch_main: ${BUILD}/sat_watch_main.o ${MODS} | ${DEPS}
	${CXX} -o $@ $^ ${LDFLAGS}

# ${BUILD}/test_main: ${BUILD}/test_main.o ${MODULES} ${TEST_MODS} | ${DEPS}
# 	${CXX} -o $@ $^ ${LDFLAGS} ${EVE_TEST_LIB}

${BUILD}/%_main.o: src/%_main.cc | ${DEPS}
	${CXX} ${CXXFLAGS} -o $@ -c $^


