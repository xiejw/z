SRC     = src
BUILD   = .build

FORMAT  = src/format.tex
DEPS    = src/algo.tex
OUTS    = $(subst tex,pdf,$(subst src,${BUILD},${DEPS}))

compile: ${OUTS}

${BUILD}/%.pdf: ${SRC}/%.tex ${FORMAT} | ${BUILD}
	pdftex -output-directory $(shell dirname $@) $<

${BUILD}:
	mkdir -p ${BUILD}
