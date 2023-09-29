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

### Docker on arm64

A little faster (but not so strong)
```
docker run --rm -ti xiejw/connect_4_pt_medium
```

A little slower (but stronger)
```
docker run --rm -ti xiejw/connect_4_pt
```

### Local Python

- Download the pytorch (binary) state file, if absent, from
  [here](https://github.com/xiejw/z/releases).

- Sanity check python dependencies and compiler:

        make check

- Then run:

        make run

