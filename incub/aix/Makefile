# === Configurations -----------------------------------------------------------
#
UNAME     = $(shell uname)
BUILD     = .build
SHELL     = bash

# === CXXFLAGS and LDFLAGS -----------------------------------------------------
#
CXXFLAGS += -std=c++23 -Wall -Werror -pedantic -Wextra -Wfatal-errors -Wconversion
LDFLAGS  += -lm


ifdef RELEASE
CXXFLAGS += -DNDEBUG -O3 -march=native
LDFLAGS  += -ffast-math
else
CXXFLAGS += -g
endif

ifdef ASAN
CXXFLAGS += -fsanitize=address
endif


# === Rules --------------------------------------------------------------------
#
main:  | ${BUILD}
	@${CXX} -o ${BUILD}/a.out ${CXXFLAGS} main.cc ${LDFLAGS} && ./${BUILD}/a.out

${BUILD}:
	@mkdir -p ${BUILD}

fmt:
	@~/Workspace/y/tools/clang_format_all.sh .

clean:
	rm -rf ${BUILD}
