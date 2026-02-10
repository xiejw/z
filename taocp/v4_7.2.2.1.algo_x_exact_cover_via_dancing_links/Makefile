# === Configurations -----------------------------------------------------------
#
UNAME     = $(shell uname)
BUILD     = .build
SHELL     = bash

# === ZION ---------------------------------------------------------------------
#
ZION_PATH = zion_c
include ${ZION_PATH}/zion.mk

# === CFLAGS and LDFLAGS -------------------------------------------------------
#
CFLAGS   += -std=c11 -Wall -Werror -pedantic -Wextra -Wfatal-errors -Wconversion
LDFLAGS  += -lm

CFLAGS   += -I.
CFLAGS   += -I${ZION_PATH}/include

ifdef RELEASE
CFLAGS   += -DNDEBUG -O3 -march=native
else
CFLAGS   += -g
endif

ifdef ASAN
CFLAGS   += -fsanitize=address
endif

MODS     += ${BUILD}/dlink.o

# === Rules --------------------------------------------------------------------
#
.PHONY: release compile test clean fmt

.DEFAULT_GOAL := compile

compile: ${MODS} ${LIBZION} | ${BUILD}

run: ${MODS} ${LIBZION} | ${BUILD}
	${CC} -o ${BUILD}/a.out ${CFLAGS} ${LDFLAGS} $^ sudoku.c && ./${BUILD}/a.out

release: compile

test: ${LIBZION} ${MODS} | ${BUILD}
	${CC} -o ${BUILD}/test ${CFLAGS} ${LDFLAGS} $^ dlink_test.c && ./${BUILD}/test

# === Mods ---------------------------------------------------------------------
#
${BUILD}/%.o: %.c | ${BUILD}
	@echo -n "compile --- `basename $@`" && ${CC} -o $@ ${CFLAGS} -c $^ && echo "."

# === House Keeping ------------------------------------------------------------
#
${BUILD}:
	mkdir -p ${BUILD}

fmt:
	~/Workspace/y/tools/clang_format_all.sh .

clean:
	rm -rf ${BUILD}
