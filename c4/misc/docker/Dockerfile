# vim: filetype=dockerfile
FROM xiejw/debian

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        python3-pip python3 python3-venv \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# debian 12 and above does not allow global pip installs anymore. Virtual env
# is needed.
RUN python3 -m venv /pyenv
ENV PY=/pyenv/bin/python3

RUN /pyenv/bin/pip3 install numpy==1.26.0 MarkupSafe==2.1.3
RUN /pyenv/bin/pip3 install torch==2.0.1 --index-url https://download.pytorch.org/whl/cpu

RUN mkdir -p /workdir/cmd /workdir/.build

COPY cmd/play_with_human.py /workdir/cmd
COPY lib     /workdir/lib

COPY misc/docker/run.sh /workdir

# Passed by `build` command line flag
ARG WEIGHT_FILE

COPY ${WEIGHT_FILE} /workdir/.build/resnet.pt.state
ENV WEIGHT_FILE=/workdir/.build/resnet.pt.state

WORKDIR /workdir

CMD ["bash", "run.sh"]
