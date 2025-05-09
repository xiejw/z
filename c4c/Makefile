BUILD     = .build
DATA_FILE = https://github.com/xiejw/z/releases/download/v0.0.3/tensor_data.bin

CFLAGS   += -std=c99 -Wall -Werror -pedantic -Wextra -Wfatal-errors -Wconversion
LDFLAGS  += -lm

ifdef RELEASE
CFLAGS   += -DNDEBUG -O3 -march=native
LDFLAGS  += -ffast-math
else
CFLAGS   += -g
endif

# CFLAGS  += -fsanitize=address

compile: ${BUILD}/tensor_data.bin | ${BUILD}
	clang -o ${BUILD}/a.out ${CFLAGS} ${LDFLAGS} main.c && ./${BUILD}/a.out

${BUILD}/tensor_data.bin: | ${BUILD}
	wget -q --show-progress -O $@ ${DATA_FILE} && \
		sha256sum -c checksum.txt

${BUILD}:
	mkdir -p ${BUILD}

fmt:
	~/Workspace/y/tools/clang_format_all.sh .

clean:
	rm -rf ${BUILD}
