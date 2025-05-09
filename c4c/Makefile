CFLAGS  += -std=c99 -Wall -Werror -pedantic -Wextra -Wfatal-errors -Wconversion
LDFLAGS += -lm

ifdef RELEASE
CFLAGS  += -DNDEBUG -O3 -march=native
LDFLAGS += -ffast-math
else
CFLAGS  += -g
endif

# CFLAGS  += -fsanitize=address

compile:
	clang -o a.out ${CFLAGS} ${LDFLAGS} main.c && ./a.out

fmt:
	~/Workspace/y/tools/clang_format_all.sh .
