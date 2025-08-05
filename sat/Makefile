# === Configurations -----------------------------------------------------------
#
UNAME     = $(shell uname)
BUILD     = .build
BUILD_OBJ = ${BUILD}/objs
SHELL     = bash
SRC       = src

ZION_PATH = ../../y/ann/zion/cc

# === CXXFLAGS and LDFLAGS -----------------------------------------------------
#
CXXFLAGS        += -std=c++23 -Wall -Werror -pedantic -Wextra -Wfatal-errors -Wconversion
LDFLAGS         += -lm

CXXFLAGS        += -I${ZION_PATH}/include

DEPS            += ${BUILD} ${BUILD_OBJ}

# Release
ifdef RELEASE
CXXFLAGS += -DNDEBUG -O3 -march=native
CXXFLAGS += -fno-exceptions
# -fno-rtti
LDFLAGS  += -ffast-math
else
CXXFLAGS += -g
endif

# Asan
ifdef ASAN
CXXFLAGS += -fsanitize=address
endif

# === Mods ---------------------------------------------------------------------
#
MODS            += ${BUILD_OBJ}/sat_solver.o
MODS            += ${BUILD_OBJ}/sat_watch.o

# === Rules --------------------------------------------------------------------
#

.DEFAULT_GOAL   = run

compile: ${MODS} | ${DEPS}

run: compile
	${BUILD}/sat_watch_main

release: clean
	make RELEASE=1 compile

test: compile

# ------------------------------------------------------------------------------
# Rules for sub-modules
#
# ------------------------------------------------------------------------------
# Cmds
#
compile: ${BUILD}/sat_watch_main

${BUILD}/sat_watch_main: ${BUILD}/sat_watch_main.o ${MODS} | ${DEPS}
	${CXX} -o $@ $^ ${LDFLAGS}

${BUILD_OBJ}/%.o: ${SRC}/%.cc ${SRC}/*.h | ${DEPS}
	${CXX} -o $@ ${CXXFLAGS} -c $<

${BUILD}/%_main.o: src/%_main.cc | ${DEPS}
	${CXX} -o $@ ${CXXFLAGS} -c $^

clean:
	rm -rf ${BUILD}

${BUILD}:
	@mkdir -p ${BUILD}

${BUILD_OBJ}:
	@mkdir -p ${BUILD_OBJ}

fmt:
	@~/Workspace/y/tools/clang_format_all.sh .

