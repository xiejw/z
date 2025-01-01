#! /bin/bash
#
# vim: ft=bash
#
# This is the launcher to prepare the env to play or run sanity check.
#
# NOTE(2024-12): Use python3.12 on macOs. Torch does not have 3.13 build yet.
#
# DEBUG: set -x
set -e

# === --- Define global variables ------------------------------------------ ===
#
BUILD=.build
VENV_DIR=${BUILD}/venv
WEIGHTS_DIR=${BUILD}/weights

CONFIGURE_MK_FILE=configure.mk
WEIGHTS_REMOTE_FILE=https://github.com/xiejw/z/releases/download/v0.0.1/c4-resnet.pt.tar.gz
PMT="==>"

# === --- Define functions ------------------------------------------------- ===
#
# Select the best Python based on platform
select_py() {
  if [ "$(uname)" == "Darwin" ]; then
    PY=$(which python3.12 || which python3 || which python)
  else
    PY=$(which python3 || which python)
  fi
  echo "${PMT} Python: ${PY}"
}

# Install deps.
pip_install() {
  # Special ones.
  if [ "$(uname)" == "Darwin" ] && [ "$(uname -p)" == "i386" ]; then
    echo "${PMT} Install old torch for macos x64_64"
    # See
    # - https://github.com/pytorch/pytorch/issues/114602
    # - https://dev-discuss.pytorch.org/t/pytorch-macos-x86-builds-deprecation-starting-january-2024/1690
    # - https://www.python.org/downloads/macos/
    pip install https://files.pythonhosted.org/packages/a0/ef/c09d5e8739f99ed99c821a468830b06ac0af0d21e443afda8d2459fdc50a/torch-2.2.0-cp312-none-macosx_10_9_x86_64.whl
    pip install 'numpy<2'
  else
    pip install torch==2.5.1 --index-url https://download.pytorch.org/whl/cpu
    pip install torch numpy==2.2.1
  fi

  # Common ones.
  pip install tqdm
}

# Prepare venv and install deps.
install_venv_locally() {
  if [ ! -d ${VENV_DIR} ]; then
    echo "${PMT} Create venv dir: ${VENV_DIR}"
    mkdir -p ${VENV_DIR}
    ${PY} -m venv ${VENV_DIR}
    source ${VENV_DIR}/bin/activate
    select_py
    pip_install
  else
    source ${VENV_DIR}/bin/activate
    select_py
  fi
}

# Install weights from github release.
install_weights_locally() {
  if [ ! -d ${WEIGHTS_DIR} ]; then
    echo "${PMT} Create weights dir: ${WEIGHTS_DIR}"
    mkdir -p ${WEIGHTS_DIR}
    wget -O ${WEIGHTS_DIR}/state.tar.gz ${WEIGHTS_REMOTE_FILE}
    tar  -C ${WEIGHTS_DIR} -xvf ${WEIGHTS_DIR}/state.tar.gz
  fi
}

# Generate CONFIGURE_MK_FILE
generate_configure_mk() {
  if [ ! -f ${CONFIGURE_MK_FILE} ]; then
    echo "${PMT} ${CONFIGURE_MK_FILE}"
    ${PY}  configure.py --model_dir ${WEIGHTS_DIR}/
  fi
}

# === --- Set up the Python venv, weights file, and configure.mk ----------- ===
#
select_py
install_venv_locally
install_weights_locally
generate_configure_mk

# === --- Play or Sanity check --------------------------------------------- ===
#
if [ ! -z "${C4_SANITY_CHECK}" ]; then
  echo "${PMT} Run sanity check."
  make test
else
  make play
fi

