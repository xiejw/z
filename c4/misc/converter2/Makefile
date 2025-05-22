ROOT = ../..
CFLAGS  += -std=c99 -Wall -Werror -pedantic -Wextra -Wfatal-errors -Wconversion
# CFLAGS  += -g
# CFLAGS  += -fsanitize=address
LDFLAGS += -lm -ffast-math

# CFLAGS +=

run:
	mkdir -p .build && \
	PYTHONPATH="${ROOT}/lib" C4_STATE_FILE=${ROOT}/.build/weights/c4-resnet-5x5.pt.state ${ROOT}/.build/venv/bin/python converter.py

compile:
	clang -o a.out ${CFLAGS} ${LDFLAGS} main.c && ./a.out

bench_blas: CFLAGS += -DBLAS=1 -lblas
bench_blas:
	clang -o a.out conv2d.c -O3 -march=native ${CFLAGS} ${LDFLAGS}  && ./a.out

bench_blis: CFLAGS +=  -lblas -isystem ${HOME}/Workspace/build/blis/include/firestorm ${HOME}/Workspace/build/blis/lib/firestorm/libblis.a
bench_blis:
	clang -o a.out conv2d.c -O3 -march=native ${CFLAGS} ${LDFLAGS}  && ./a.out

bench_mac: CFLAGS +=  -DMAC=1 -DACCELERATE_NEW_LAPACK  -framework Accelerate
bench_mac:
	clang -o a.out conv2d.c -O3 -march=native ${CFLAGS} ${LDFLAGS}  && ./a.out

fmt:
	~/Workspace/y/tools/clang_format_all.sh .
