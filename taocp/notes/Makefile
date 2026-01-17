BUILD = .build

run: | ${BUILD}
	pdflatex -output-directory=${BUILD} main.tex && open ${BUILD}/main.pdf

${BUILD}:
	@mkdir -p ${BUILD}
