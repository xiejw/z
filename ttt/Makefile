CFLAGS += -Wall -Werror -pedantic -Wextra -Wfatal-errors -Wconversion
CFLAGS += -O3 -ffast-math -lm

run:
	mkdir -p .build && \
	cc ttt.c -o .build/ttt ${CFLAGS} && \
	.build/ttt

fmt:
	~/Workspace/y/tools/clang_format_all.sh .
