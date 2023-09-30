# compile python c ext and clean it
source /pyenv/bin/activate

PKG="make libstdc++-12-dev g++"

apt update \
    && apt install -y --no-install-recommends ${PKG}  \
    && make -C /workdir/cc C4_FILE_PATH=/workdir/.build/traced_resnet_model.pt C4_MODEL_ON_CPU=1 \
    && cp /workdir/cc/*.so /workdir/ \
    && rm -rf /workdir/cc \
    && apt remove -y ${PKG} \
    && apt autoremove -y \
    && apt clean \
    && rm -rf /var/lib/apt/lists/*
