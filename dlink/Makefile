# === Configurations -----------------------------------------------------------
#
UNAME     = $(shell uname)
BUILD     = .build

# === CFLAGS and LDFLAGS -------------------------------------------------------
#
CFLAGS   += -std=c11 -Wall -Werror -pedantic -Wextra -Wfatal-errors -Wconversion
LDFLAGS  += -lm

CFLAGS   += -I../

ifdef RELEASE
CFLAGS   += -DNDEBUG -O3 -march=native
else
CFLAGS   += -g
endif

ifdef ASAN
CFLAGS   += -fsanitize=address
endif


# === Rules --------------------------------------------------------------------
#
compile: | ${BUILD}
	clang -o ${BUILD}/a.out ${CFLAGS} ${LDFLAGS} main.c && ./${BUILD}/a.out

release: test

test: CFLAGS += -DADT_TEST_H_=1
test: | ${BUILD}
	clang -o ${BUILD}/test ${CFLAGS} ${LDFLAGS} dlink.c && ./${BUILD}/test

${BUILD}:
	mkdir -p ${BUILD}

fmt:
	~/Workspace/y/tools/clang_format_all.sh .

clean:
	rm -rf ${BUILD}
