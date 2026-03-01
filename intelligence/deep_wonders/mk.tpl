# vim: ft=make
#
# Version 1 of common Makefile used in this project
#
# Opinioned about the structure of code bases
#
#   cmd/   # All binary main files
#   src/   # All dependendcies
#
# Call side defines BINS and SRC_OBJS
#
BUILD       = .build
BUILD_OBJS  = ${BUILD}/objs

CXXFLAGS   += -std=c++17
CXXFLAGS   += -Wall -Werror -pedantic -Wextra -Wfatal-errors -Wconversion
CXXFLAGS   += -fno-rtti -fno-exceptions
CXXFLAGS   += -Isrc

SRC_DEPS    += $(wildcard src/*.cc)
SRC_DEPS    += $(wildcard src/*.h)
SRC_DEPS    += $(wildcard cmd/*.cc)

ifdef RELEASE
CXXFLAGS   += -DNDEBUG -O3 -march=native
else
CXXFLAGS   += -g
endif

ifdef ASAN
LDFLAGS  += -fsanitize=address
endif

# === --- Actions----------------------------------------------------------- ===

all: $(addprefix ${BUILD}/,${BINS})

compile: all

release: clean
	make RELEASE=1 compile

# === --- Rules ------------------------------------------------------------ ===

${BUILD}/%: ${BUILD}/cmd_%.o ${SRC_OBJS} | ${BUILD}
	${CXX} ${LDFLAGS} -o $@ $^

${BUILD}/cmd_%.o:  cmd/%.cc ${SRC_DEPS} | ${BUILD}
	${CXX} ${CXXFLAGS} -o ${shell printf "%-30s" $@} -c $<

${BUILD_OBJS}/%.o: src/%.cc ${SRC_DEPS} | ${BUILD_OBJS}
	${CXX} ${CXXFLAGS} -o ${shell printf "%-30s" $@} -c $<

# === --- House Keeping ---------------------------------------------------- ===

${BUILD}:
	@mkdir -p $@

${BUILD_OBJS}: ${BUILD}
	@mkdir -p $@

fmt:
	~/Workspace/y/tools/clang_format_all.sh .

clean:
	rm -rf ${BUILD}
