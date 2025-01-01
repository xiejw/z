#! /bin/sh
# vim: ft=bash
# This is the step to prepare the env to play.
#
# NOTE(2024-12): Use python3.12 on macOs. Torch does not have 3.13 build yet.
#
set -e
# set -x

PMT="==>"
BUILD=.build
VENV_DIR=${BUILD}/venv
WEIGHTS_DIR=${BUILD}/weights

py_select() {
  if [ "$(uname)" == "Darwin" ]; then
    PY=$(which python3.12 || which python)
  else
    PY=$(which python3.12 || which python)
  fi
  echo "${PMT} Python: ${PY}"
}

pip_install() {
  if [ "$(uname)" == "Darwin" ] && [ "$(uname -p)" == "i386" ]; then
    echo "${PMT} Install old torch for macos x64_64"
    pip install https://files.pythonhosted.org/packages/a0/ef/c09d5e8739f99ed99c821a468830b06ac0af0d21e443afda8d2459fdc50a/torch-2.2.0-cp312-none-macosx_10_9_x86_64.whl
    pip install numpy
  else
    pip install torch numpy
  fi
}

py_select

if [ ! -d ${VENV_DIR} ]; then
  echo "${PMT} Create venv dir: ${VENV_DIR}"
  mkdir -p ${VENV_DIR}
  ${PY} -m venv ${VENV_DIR}
  pip_install
fi

# === --- Set up the venv for python and pip ------------------------------- ===
#
source ${VENV_DIR}/bin/activate
PY=$(which python)
echo "${PMT} Python: ${PY}"

if [ ! -d ${WEIGHTS_DIR} ]; then
  echo "${PMT} Create weights dir: ${WEIGHTS_DIR}"
  mkdir -p ${WEIGHTS_DIR}
  wget -O ${WEIGHTS_DIR}/state.tar.gz https://github.com/xiejw/z/releases/download/v0.0.1/c4-resnet.pt.tar.gz
  tar  -C ${WEIGHTS_DIR} -xvf ${WEIGHTS_DIR}/state.tar.gz
fi
python configure.py --model_dir ${WEIGHTS_DIR}/
make play

