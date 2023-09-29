# To create tar.gz for release
#   tar -czvf c4-resnet.pt.state.tar.gz c4-resnet.pt.state

ifndef ${C4_STATE_FILE}
	C4_STATE_FILE = ${HOME}/Desktop/c4-resnet-5x5.pt.state
endif

PY            = python3
PY_LIB_PATH   = PYTHONPATH="lib:cc"

DOCKER_ASSETS = .docker_assets

PYBIND_CC_MOD = cc/xai_c4${shell python3-config --extension-suffix}
PYBIND_CC_SRC = cc/c4.cc

.PHONY: run check default

default: check run

run: check_pybind_cc
	${PY_LIB_PATH} C4_STATE_FILE=${C4_STATE_FILE} ${PY} cmd/play_with_human.py

check:
	python3 -c "import pybind11; import numpy" && \
		python3 -c "import torch; assert torch.backends.mps.is_available()" && \
		clang --version > /dev/null && \
		echo "passed"

check_pybind_cc: ${PYBIND_CC_MOD}

${PYBIND_CC_MOD}: ${PYBIND_CC_SRC}
	make -C cc

clean:
	rm -rf ${DOCKER_ASSETS} && make -C cc clean

build_docker:
	mkdir -p ${DOCKER_ASSETS} && \
		cp -r lib cmd misc ${DOCKER_ASSETS} && \
		cp ${C4_STATE_FILE} ${DOCKER_ASSETS}/resnet.pt.state && \
	docker build -t xiejw/connect_4_pt \
		--build-arg WEIGHT_FILE=resnet.pt.state \
		-f misc/docker/Dockerfile \
		${DOCKER_ASSETS}
