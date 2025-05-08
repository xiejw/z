CFLAGS  += -std=c99 -Wall -Werror -pedantic -Wextra -Wfatal-errors -Wconversion
CFLAGS  += -g
# CFLAGS  += -fsanitize=address
LDFLAGS += -lm -ffast-math

compile:
	clang -o a.out ${CFLAGS} ${LDFLAGS} main.c && ./a.out

fmt:
	~/Workspace/y/tools/clang_format_all.sh .
