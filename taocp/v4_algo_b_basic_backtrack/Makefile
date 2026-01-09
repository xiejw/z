BUILD       = .build
BUILD_OBJS  = ${BUILD}/objs

INSTALL     = ~/Workspace/.bin

CXXFLAGS   += -std=c++17
CXXFLAGS   += -Wall -Werror -pedantic -Wextra -Wfatal-errors -Wconversion
CXXFLAGS   += -fno-rtti -fno-exceptions
CXXFLAGS   += -Isrc

ifdef RELEASE
CXXFLAGS   += -DNDEBUG -O3 -march=native
# LDFLAGS  += -ffast-math
else
CXXFLAGS   += -g
endif

ifdef ASAN
LDFLAGS  += -fsanitize=address
endif

MODS    += ${BUILD}/cmd_main.o
MODS    += ${BUILD_OBJS}/log.o

run: compile
	${BUILD}/a.out

compile: ${BUILD}/a.out

release: clean
	make RELEASE=1 compile

# === ---------------------------------------------------------------------- ===

${BUILD}/a.out: ${MODS} | ${BUILD}
	${CXX} ${LDFLAGS} -o ${BUILD}/a.out ${MODS}

${BUILD}/cmd_main.o: cmd/main.cc | ${BUILD}
	${CXX} ${CXXFLAGS} -o ${shell printf "%-30s" $@} -c $^

${BUILD_OBJS}/%.o:   src/%.cc | ${BUILD_OBJS}
	${CXX} ${CXXFLAGS} -o ${shell printf "%-30s" $@} -c $^

# === ---------------------------------------------------------------------- ===

${BUILD}:
	@mkdir -p $@

${BUILD_OBJS}: ${BUILD}
	@mkdir -p $@

fmt:
	~/Workspace/y/tools/clang_format_all.sh .

clean:
	rm -rf ${BUILD}

