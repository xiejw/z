This code is converting the Torch c4 model to pure c implementation. This is a
project in 2025 to write a pure c implementation.

To check accuracy, run
```
make          # Run python code to dump the weight, input and outputs.
make compile  # Run c code to re-compute and compare outputs.
```

During final weights dump, comment out the input and outputs in Python tensor
dumps.  You can follow `UPDATE_BEFORE_FINAL_DUMP` to find them.
