CFLAGS  += -std=c99 -Wall -Werror -pedantic -Wextra -Wfatal-errors -Wconversion
LDFLAGS += -lm

ifdef RELEASE
CFLAGS  += -DNDEBUG -O3 -march=native
LDFLAGS += -ffast-math
else
CFLAGS  += -g
endif

# CFLAGS  += -fsanitize=address

compile: tensor_data.bin
	clang -o a.out ${CFLAGS} ${LDFLAGS} main.c && ./a.out

tensor_data.bin:
	wget -O $@ https://github.com/xiejw/z/releases/download/v0.0.2/tensor_data.bin

fmt:
	~/Workspace/y/tools/clang_format_all.sh .

clean:
	rm -rf a.out*
