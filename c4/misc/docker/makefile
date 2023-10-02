DOCKER_ASSETS    = ../../.docker_assets

ROOT_DIR         = ../..

DOCKER_DIRS      = ${ROOT_DIR}/lib ${ROOT_DIR}/cmd ${ROOT_DIR}/misc
DOCKER_PAR_DIRS  = ${ROOT_DIR}/cc ${DOCKER_DIRS}

#
# docker
#
build_docker: clean
	mkdir -p ${DOCKER_ASSETS} &&                                    \
		cp -r ${DOCKER_DIRS} ${DOCKER_ASSETS} &&                \
		cp ${C4_STATE_FILE} ${DOCKER_ASSETS}/resnet.pt.state && \
	DOCKER_SCAN_SUGGEST=false DOCKER_CLI_HINTS=false docker         \
	        build -t xiejw/connect_4_pt                             \
		--build-arg WEIGHT_FILE=resnet.pt.state                 \
		-f Dockerfile                                           \
		${DOCKER_ASSETS}

build_docker_par: clean
	mkdir -p ${DOCKER_ASSETS} &&                                          \
		cp -r ${DOCKER_PAR_DIRS} ${DOCKER_ASSETS} &&                  \
		cp ${C4_FILE_PATH} ${DOCKER_ASSETS}/traced_resnet_model.pt && \
	DOCKER_SCAN_SUGGEST=false DOCKER_CLI_HINTS=false docker               \
	        build -t xiejw/connect_4_par_pt_lookahead_onestep             \
		--build-arg WEIGHT_FILE=traced_resnet_model.pt                \
		-f Dockerfile.par                                             \
		${DOCKER_ASSETS}

clean:
	rm -rf ${DOCKER_ASSETS}
