BUILD = .build

run: | ${BUILD}
	pdflatex -output-directory=${BUILD} main.tex && \
		open ${BUILD}/main.pdf               && \
		osascript                               \
		        -e 'tell application "Preview" to activate' \
		        -e 'tell application "System Events" to keystroke (ASCII character 31) using command down'

${BUILD}:
	@mkdir -p ${BUILD}
