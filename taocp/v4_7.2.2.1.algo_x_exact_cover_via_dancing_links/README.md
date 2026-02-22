## TAOCP v4 Dancing Link

### Dancing Links

Dancing link is an advanced data structure for backtrack algorithms.  It is very
efficient for _undo_ operations. One example for its use cases is solving Exact
Cover Problems.

|Algorithm List| Page |
| :--- | :---|
| V4 7.2.2.1X Exact Cover via Dancing Links| Page 69 |

Reference Books and Papers
- [Volume 4B](https://www.amazon.com/Art-Computer-Programming-Combinatorial-Information/dp/0201038064).
- [Volume 4 Pre-fascicle 5c](https://www.amazon.com/Art-Computer-Programming-Fascicle-Preliminaries/dp/0134671791).
- [Dancing Links Paper][1].

#### Use Case: Sudoku

See code, [`main.cc`](cmd/main.cc), for an interesting example demonstrates
Dancing Links algorithm. It is based on [Dancing Links Paper][1] by Donald E.
Knuth.

[1]: https://arxiv.org/pdf/cs/0011047.pdf

### Data Structures and Algorithms

#### A Dancing Link Table with Pre-allocated Nodes

A dancing-link table consists of items and options:
- All items must be covered exactly once.
- An option can cover multiple items. It is represented as a group of nodes each
  covering one item.

For example, to cover all rows and columns exactly once in a `3x3` board game
with stones, like
```
  1 2 3
 +-+-+-+
1| | | |
 +-+-+-+
2| | | |
 +-+-+-+
3| | | |
 +-+-+-+,
```
we could design 6 items as
```
r1 r2 r3 c1 c2 c3.
```
where `r` is row and `c` is column.

With that, if a stone is placed at row 1 (`r1`) and column 2 (`c2`), called an
option, then visually 2 nodes for this option will be inserted into the
table like
```
            r1 r2 r3 c1 c2 c3             1 2 3
            ^           ^                +-+-+-+
            |           |               1| |x| |
            v           v                +-+-+-+
(r1,c2)     + <-------> +               2| | | |
                                         +-+-+-+
                                        3| | | |
                                         +-+-+-+.
```
Note that the 2 nodes are horizontally linked together so they can find each
other and each node is vertically linked with its covered item so once a item is
covered, all options (and their nodes) can be looked up quickly.

To see how it helps, the conflicting options `(r1,c1)` and `(r1,c2)` are
encoded in the table like
```
            r1 r2 r3 c1 c2 c3             1 2 3
            ^        ^  ^                +-+-+-+
            |        |  |               1|x|x| |
            |        |  |                +-+-+-+
            v        v  |               2| | | |
(r1,c1)     + <----> +  |                +-+-+-+
            ^           |               3| | | |
            |           |                +-+-+-+.
            v           v
(r1,c2)     + <-------> +
```
When the option `(r1,c1)` is selected, item `r1` will be covered and all options
(and nodes) it vertically linked, e.g., `(r1,c2)`, are unlinked temporarily.
This means `(r1,c2)` will not be part of the search branch anymore, until
backtracking.

