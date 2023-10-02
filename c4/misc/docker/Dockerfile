# vim: filetype=dockerfile
FROM xiejw/torch

RUN  mkdir -p /workdir/cmd /workdir/.build

COPY cmd/play_with_human.py /workdir/cmd
COPY lib     /workdir/lib

COPY misc/docker/run.sh /workdir

# Passed by `build` command line flag
ARG  WEIGHT_FILE

COPY ${WEIGHT_FILE} /workdir/.build/resnet.pt.state
ENV  WEIGHT_FILE=/workdir/.build/resnet.pt.state

WORKDIR /workdir

CMD ["bash", "run.sh"]
