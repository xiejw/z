Connect Four
============

According to Wikipedia:

> Connect Four is a two-player connection game in which the players first choose
a color and then take turns dropping one colored disc from the top into a
seven-column, six-row vertically suspended grid. The pieces fall straight down,
occupying the lowest available space within the column. The objective of the
game is to be the first to form a horizontal, vertical, or diagonal line of four
of one's own discs.

![ConnectFour](./misc/images/c4.gif)

Let's Play
==========

### Youtube Video

See [Youtube here](https://youtu.be/tGG8I9GnisM)

### How to Start
```
./run.py   # For both bootstrap and daily run.
```

### Other Ways
<details>
  <summary>Click me</summary>

  #### Docker on arm64/amd64

  Few choices

  - [arm64] Super fast but only look ahead one step.

          docker run --rm -ti xiejw/connect_4_par_pt_lookahead_onestep

  - [arm64/amd64] A little slow, but should be a good player

          [arm64]
          docker run --rm -ti xiejw/connect_4_pt_medium
          [x86_64]
          docker run --rm -ti xiejw/connect_4_pt_medium_x86_64

  - [arm64] Quite slow, but qutie strong

          docker run --rm -ti xiejw/connect_4_pt

  #### Local Python (macOS with MPS torch backend)

  _Last Test: macOS 13.6 + brew Python on May 2024_.

  - Download the pytorch (binary) state file, if absent, from
    [here](https://github.com/xiejw/z/releases).

  - Place state files at `~/Desktop`.

  - Ensure a `C` compiler is available (`clang`).

  - With macOS, ensure install python via brew.

          brew install python
          brew ls python

  - Ensure Python 3 dependencies are installed

          pip install torch numpy setuptools

  - Run bootstrap configure process (one time only)

          python configure.py

  - Then play:

          make play
 </details>
