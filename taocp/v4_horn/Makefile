# === Configurations -----------------------------------------------------------
#
UNAME     = $(shell uname)
BUILD     = .build
SHELL     = bash

# === ZION ---------------------------------------------------------------------
#
ZION_PATH = ../../zion

# === CFLAGS and LDFLAGS -------------------------------------------------------
#
CFLAGS   += -std=c11 -Wall -Werror -pedantic -Wextra -Wfatal-errors -Wconversion
LDFLAGS  += -lm

CFLAGS   += -I..
CFLAGS   += -I${ZION_PATH}/c/include

ifdef RELEASE
CFLAGS   += -DNDEBUG -O3 -march=native
LDFLAGS  += -ffast-math
else
CFLAGS   += -g
endif

ifdef ASAN
CFLAGS   += -fsanitize=address
endif

# === Rules --------------------------------------------------------------------
#
.PHONY: release compile test clean fmt

compile: | ${BUILD}
	${CC} -o ${BUILD}/horn.o ${CFLAGS} -c horn.c

release: compile

test: | ${BUILD}
	@set -e
	@${CC} -o ${BUILD}/test ${CFLAGS} ${LDFLAGS} ${LIBZION} horn.c horn_test.c  && echo -n "horn  -- " && ./${BUILD}/test
	@echo "All done."

# === House Keeping ------------------------------------------------------------
#
${BUILD}:
	mkdir -p ${BUILD}

fmt:
	~/Workspace/y/tools/clang_format_all.sh .

clean:
	rm -rf ${BUILD}
