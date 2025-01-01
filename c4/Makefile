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
PY_LIB_PATH   = PYTHONPATH="lib"

#
# actions
#
.PHONY: play check build_docker clean default

default: check play

play:
	${PY_LIB_PATH} C4_STATE_FILE=${C4_STATE_FILE} ${PY} cmd/play_with_human.py

#
# maintenance
#
clean:
	@echo "deep clean"                          && \
		make --silent -C misc/docker clean  && \
		rm -rf .build                       && \
		find . -name 'a.out' -exec rm {} \;

#
# docker
#
build_docker:
	make -C misc/docker C4_STATE_FILE=${C4_STATE_FILE} build_docker

build_docker_par:
	make -C misc/docker C4_TRACED_MODEL_FILE=${C4_TRACED_MODEL_FILE} build_docker_par
