# === Configurations -----------------------------------------------------------
#
UNAME     = $(shell uname)
BUILD     = .build
SHELL     = bash


# === CFLAGS and LDFLAGS -------------------------------------------------------
#
CFLAGS   += -std=c11 -Wall -Werror -pedantic -Wextra -Wfatal-errors -Wconversion
LDFLAGS  += -lm

CFLAGS   += -Iinclude

ifdef RELEASE
CFLAGS   += -DNDEBUG -O3 -march=native
LDFLAGS  += -ffast-math
else
CFLAGS   += -g
endif

ifdef ASAN
CFLAGS   += -fsanitize=address
endif

MODS     += ${BUILD}/vec.o
MODS     += ${BUILD}/sds.o

# === Rules --------------------------------------------------------------------
#
.PHONY: release compile test fmt clean
compile: ${MODS}

release: clean
	make RELEASE=1 libzion

libzion: ${BUILD}/libzion.a

test: ${MODS} | ${BUILD}
	@set -e
	@${CC} -o ${BUILD}/test ${CFLAGS} ${LDFLAGS} $^ src/vec_test.c   && echo -n "vec   -- " && ./${BUILD}/test
	@${CC} -o ${BUILD}/test ${CFLAGS} ${LDFLAGS} $^ src/sds_test.c   && echo -n "sds   -- " && ./${BUILD}/test
	@echo "All done."

# === Mods & Archive -----------------------------------------------------------
#
${BUILD}/%.o: src/%.c | ${BUILD}
	${CC} -o $@ ${CFLAGS} -c $^

${BUILD}/libzion.a: ${MODS}
ifdef RELEASE
	ar -cr $@ $^
else
	@echo building libzion.a must be RELEASE mode.
	exit 1
endif  # RELEASE

# === House Keeping ------------------------------------------------------------
#
${BUILD}:
	mkdir -p ${BUILD}

fmt:
	~/Workspace/y/tools/clang_format_all.sh .

clean:
	rm -rf ${BUILD}
