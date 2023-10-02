# To create tar.gz for release
#
#   tar -czvf c4-resnet.pt.tar.gz c4-resnet-5x5.pt.state traced_resnet_model.pt

#
# configs from configure.py
#
-include ./configure.mk

ifndef C4_CONFIGURE_DONE
$(error "Project is not configured. Run 'python configure.py' in top level folder")
endif

#
# configs
#

PY            = python3
PY_LIB_PATH   = PYTHONPATH="lib:cc"

PYBIND_CC_MOD = cc/xai_c4${PY_EXT_SUFFIX}
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
clean:
	@echo "deep clean"                          && \
		make --silent -C misc/docker clean  && \
		make --silent -C cc clean           && \
		find . -name 'a.out' -exec rm {} \; && \
		rm -rf configure.mk

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
	make -C misc/docker C4_TRACED_MODEL_FILE=${C4_TRACED_MODEL_FILE} build_docker_par
