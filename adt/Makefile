# === Configurations -----------------------------------------------------------
#
UNAME     = $(shell uname)
BUILD     = .build
SHELL     = bash


# === CFLAGS and LDFLAGS -------------------------------------------------------
#
CFLAGS   += -std=c11 -Wall -Werror -pedantic -Wextra -Wfatal-errors -Wconversion
LDFLAGS  += -lm

CFLAGS   += -I..

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
release: test

test: CFLAGS += -DADT_TEST_H_=1
test: | ${BUILD}
	@set -e
	@clang -o ${BUILD}/test ${CFLAGS} ${LDFLAGS} vec.c   && echo -n "vec   -- " && ./${BUILD}/test
	@clang -o ${BUILD}/test ${CFLAGS} ${LDFLAGS} sds.c   && echo -n "sds   -- " && ./${BUILD}/test
	@clang -o ${BUILD}/test ${CFLAGS} ${LDFLAGS} horn.c  && echo -n "horn  -- " && ./${BUILD}/test
	@clang -o ${BUILD}/test ${CFLAGS} ${LDFLAGS} dlink.c && echo -n "dlink -- " && ./${BUILD}/test
	@echo "All done."

${BUILD}:
	mkdir -p ${BUILD}

fmt:
	~/Workspace/y/tools/clang_format_all.sh .

clean:
	rm -rf ${BUILD}
