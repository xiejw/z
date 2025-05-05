ROOT = ../..

run:
	PYTHONPATH="${ROOT}/lib" C4_STATE_FILE=${ROOT}/.build/weights/c4-resnet-5x5.pt.state ${ROOT}/.build/venv/bin/python converter.py

compile:
	clang -o a.out -Wall main.c && ./a.out

fmt:
	~/Workspace/y/tools/clang_format_all.sh .
