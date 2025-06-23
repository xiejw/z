# === Configurations -----------------------------------------------------------
#
UNAME     = $(shell uname)
BUILD     = .build
SHELL     = bash

TOK_FILE  = ${HOME}/.llama/checkpoints/Llama3.1-8B-Instruct/tokenizer.model
ADT_ROOT  = ../..
ADT_DIR   = ${ADT_ROOT}/adt

# === CFLAGS and LDFLAGS -------------------------------------------------------
#
CFLAGS   += -std=c11 -Wall -Werror -pedantic -Wextra -Wfatal-errors -Wconversion
LDFLAGS  += -lm

# ADT
CFLAGS   += -I${ADT_ROOT}
LDFLAGS  += ${ADT_DIR}/sds.c ${ADT_DIR}/vec.c

# Pcre2
LDFLAGS  += -lpcre2-8
ifeq ($(UNAME), Darwin)
CFLAGS   += -I$(shell brew --prefix pcre2)/include
LDFLAGS  += -L$(shell brew --prefix pcre2)/lib
endif

ifdef RELEASE
CFLAGS   += -DNDEBUG -O3 -march=native
LDFLAGS  += -ffast-math
else
CFLAGS   += -g
endif

ifdef ASAN
CFLAGS   += -fsanitize=address
endif

# Enable BLAS for macOs, which uses Accelerate framework.
ifeq ($(UNAME), Darwin)
CFLAGS   += -DMACOS_ACCELERATE
CFLAGS   += -DACCELERATE_NEW_LAPACK
LDFLAGS  += -framework Accelerate
endif

ifdef BLAS
CFLAGS   += -DBLAS
LDFLAGS  += -lblas
endif

# === Knobs --------------------------------------------------------------------
#
CFLAGS   += -DTOK_FILE="\"${TOK_FILE}\""
CFLAGS   += -DLLAMA_FILE="\"/tmp/tensor_data.bin\""

# === Rules --------------------------------------------------------------------
#
main:  | ${BUILD}
	clang -o ${BUILD}/a.out ${CFLAGS} ctx.c tok.c util.c tensor.c llama.c vm.c op.c main.c ${LDFLAGS} && ./${BUILD}/a.out

${BUILD}:
	mkdir -p ${BUILD}

test: CFLAGS += -DAIC_TEST_H_
test:  | ${BUILD}
	@set -e
	@clang -o ${BUILD}/a.out ${CFLAGS} ctx.c util.c ${LDFLAGS}     && echo -n "util   -- " && ./${BUILD}/a.out
	@echo "All done."

fmt:
	~/Workspace/y/tools/clang_format_all.sh .

clean:
	rm -rf ${BUILD}
