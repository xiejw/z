# To create tar.gz for release
#   tar -czvf c4-resnet.pt.tar.gz c4-resnet.pt.state traced_resnet_model.pt

#
# configs
#
ifndef C4_STATE_FILE
	C4_STATE_FILE = ${HOME}/Desktop/c4-resnet-5x5.pt.state
endif

ifndef C4_FILE_PATH
	C4_FILE_PATH = ${HOME}/Desktop/traced_resnet_model.pt
endif

PY            = python3
PY_LIB_PATH   = PYTHONPATH="lib:cc"

PYBIND_CC_MOD = cc/xai_c4${shell python3-config --extension-suffix}
PYBIND_CC_SRC = cc/c4.cc

#
# actions
#
.PHONY: play check check_python_c_ext build_docker clean default

default: check play

play:
	${PY_LIB_PATH} C4_STATE_FILE=${C4_STATE_FILE} ${PY} cmd/play_with_human.py

play_par: check_python_c_ext
	${PY_LIB_PATH} ${PY} cmd/play_with_human.py --par

#
# maintenance
#
check:
	@echo "[makefile] checking..." && \
		python3 -c "import pybind11" && \
		python3 -c "import numpy"    && \
		python3 -c "import torch as t; assert t.backends.mps.is_available()" && \
		clang --version > /dev/null  && \
		echo "[makefile] passed..."

clean:
	make -C misc/docker clean && make -C cc clean && find . -name 'a.out' -exec rm {} \;

#
# python c extension
#
check_python_c_ext: ${PYBIND_CC_MOD}

${PYBIND_CC_MOD}: ${PYBIND_CC_SRC}
	make -C cc

#
# docker
#
build_docker:
	make -C misc/docker C4_STATE_FILE=${C4_STATE_FILE} build_docker

build_docker_par:
	make -C misc/docker C4_FILE_PATH=${C4_FILE_PATH} build_docker_par
