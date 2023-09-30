# compile python c ext and clean it
source /pyenv/bin/activate

PKG="make libstdc++-12-dev g++"

apt update \
    && apt install -y --no-install-recommends ${PKG}  \
    && make -C /workdir/cc \
    && cp /workdir/cc/*.so /workdir/ \
    && rm -rf /workdir/cc \
    && apt remove -y ${PKG} \
    && apt autoremove -y \
    && apt clean \
    && rm -rf /var/lib/apt/lists/*
