ROOT = ../..
CFLAGS += -std=c99 -Wall -Werror -pedantic -Wextra -Wfatal-errors -Wconversion
CFLAGS += -g


run:
	PYTHONPATH="${ROOT}/lib" C4_STATE_FILE=${ROOT}/.build/weights/c4-resnet-5x5.pt.state ${ROOT}/.build/venv/bin/python converter.py

compile:
	clang -o a.out ${CFLAGS} main.c && ./a.out

fmt:
	~/Workspace/y/tools/clang_format_all.sh .
