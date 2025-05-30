# === Configurations -----------------------------------------------------------
#
UNAME     = $(shell uname)
BUILD     = .build
TOK_FILE  = ${HOME}/.llama/checkpoints/Llama3.1-8B-Instruct/tokenizer.model

# === CFLAGS and LDFLAGS -------------------------------------------------------
#
CFLAGS   += -std=c99 -Wall -Werror -pedantic -Wextra -Wfatal-errors -Wconversion
LDFLAGS  += -lm

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

# === Rules --------------------------------------------------------------------
#
compile:  | ${BUILD}
	clang -o ${BUILD}/a.out ${CFLAGS} main.c ${LDFLAGS} && ./${BUILD}/a.out


${BUILD}:
	mkdir -p ${BUILD}

fmt:
	~/Workspace/y/tools/clang_format_all.sh .

clean:
	rm -rf ${BUILD}
