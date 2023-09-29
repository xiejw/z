DOCKER_ASSETS = .docker_assets

ROOT_DIR      = ../..

DOCKER_DIRS   = ${ROOT_DIR}/lib ${ROOT_DIR}/cmd ${ROOT_DIR}/misc

#
# docker
#
build_docker:
	mkdir -p ${DOCKER_ASSETS} && \
		cp -r ${DOCKER_DIRS} ${DOCKER_ASSETS} && \
		cp ${C4_STATE_FILE} ${DOCKER_ASSETS}/resnet.pt.state && \
	docker build -t xiejw/connect_4_pt \
		--build-arg WEIGHT_FILE=resnet.pt.state \
		-f Dockerfile \
		${DOCKER_ASSETS}

clean:
	rm -rf ${DOCKER_ASSETS}
