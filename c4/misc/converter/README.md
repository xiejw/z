This folder contains the code
- `00_main.py`: converting the original Keras code to PyTorch model code,
- `01_main.py`: converting the PyTorch model code using 4x4 kernel size to 5x5
  kernel size. 4x4 seems a mistake in Keras model code since day 0.
- `02_main.py`: testing the PyTorch code by loading the saved state. This is the
  code used by c4 agent later. In addition, it dumps a saved model to cc code
  later.
- `02_main.cc`: testing Torch cc code to load saved model.
