ROOT = ../..
CFLAGS  += -std=c99 -Wall -Werror -pedantic -Wextra -Wfatal-errors -Wconversion
# CFLAGS  += -g
# CFLAGS  += -fsanitize=address
LDFLAGS += -lm -ffast-math

# CFLAGS += -DACCELERATE_NEW_LAPACK  -framework Accelerate

run:
	mkdir -p .build && \
	PYTHONPATH="${ROOT}/lib" C4_STATE_FILE=${ROOT}/.build/weights/c4-resnet-5x5.pt.state ${ROOT}/.build/venv/bin/python converter.py

compile:
	clang -o a.out ${CFLAGS} ${LDFLAGS} main.c && ./a.out

bench: CFLAGS +=  -lblas -isystem ${HOME}/Workspace/build/blis/include/firestorm ${HOME}/Workspace/build/blis/lib/firestorm/libblis.a
bench:
	clang -o a.out conv2d.c -O3 -march=native ${CFLAGS} ${LDFLAGS}  && ./a.out

fmt:
	~/Workspace/y/tools/clang_format_all.sh .
