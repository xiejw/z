# === Configurations -----------------------------------------------------------
#
UNAME     = $(shell uname)
BUILD     = .build
SHELL     = bash

# === ZION ---------------------------------------------------------------------
#
ZION_PATH = ../../zion/c
include ${ZION_PATH}/zion.mk

# === CFLAGS and LDFLAGS -------------------------------------------------------
#
CFLAGS   += -std=c11 -Wall -Werror -pedantic -Wextra -Wfatal-errors -Wconversion
LDFLAGS  += -lm

CFLAGS   += -I..
CFLAGS   += -I${ZION_PATH}/include

ifdef RELEASE
CFLAGS   += -DNDEBUG -O3 -march=native
LDFLAGS  += -ffast-math
else
CFLAGS   += -g
endif

ifdef ASAN
CFLAGS   += -fsanitize=address
endif

MODS      = ${BUILD}/horn.o

# === Rules --------------------------------------------------------------------
#
.PHONY: release compile test clean fmt

compile: ${LIBZION} | ${BUILD}

release: compile

test: ${MODS} ${LIBZION} | ${BUILD}
	@set -e
	@${CC} -o ${BUILD}/test ${CFLAGS} ${LDFLAGS} $^ horn_test.c  && echo -n "test    --- horn   --- " && ./${BUILD}/test
	@echo "All done."

# === Mods ---------------------------------------------------------------------
#
${BUILD}/%.o: %.c | ${BUILD}
	@echo -n "compile --- `basename $@`" && ${CC} -o $@ ${CFLAGS} -c $^ && echo "."

# === House Keeping ------------------------------------------------------------
#
${BUILD}:
	@mkdir -p ${BUILD}

fmt:
	~/Workspace/y/tools/clang_format_all.sh .

clean:
	rm -rf ${BUILD}
