# To create tar.gz for release
#   tar -czvf c4-resnet.pt.tar.gz c4-resnet.pt.state traced_resnet_model.pt

#
# configs
#
ifndef ${C4_STATE_FILE}
	C4_STATE_FILE = ${HOME}/Desktop/c4-resnet-5x5.pt.state
endif

PY            = python3
PY_LIB_PATH   = PYTHONPATH="lib:cc"

DOCKER_ASSETS = .docker_assets

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

check:
	@echo "[makefile] checking..." && \
		python3 -c "import pybind11" && \
		python3 -c "import numpy"    && \
		python3 -c "import torch as t; assert t.backends.mps.is_available()" && \
		clang --version > /dev/null  && \
		echo "[makefile] passed..."

clean:
	rm -rf ${DOCKER_ASSETS} && make -C cc clean

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
	mkdir -p ${DOCKER_ASSETS} && \
		cp -r lib cmd misc ${DOCKER_ASSETS} && \
		cp ${C4_STATE_FILE} ${DOCKER_ASSETS}/resnet.pt.state && \
	docker build -t xiejw/connect_4_pt \
		--build-arg WEIGHT_FILE=resnet.pt.state \
		-f misc/docker/Dockerfile \
		${DOCKER_ASSETS}
