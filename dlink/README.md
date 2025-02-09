### Data Structures and Algorithms

#### A Dancing Link Table with Pre-allocated Nodes

A dancing-link table consists of headers (aka items) and options.  All headers
should be covered exactly once. For example, to cover all rows and columns
exactly once in a `3x3` board game with stones, like
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
we could design 6 headers as
```
r1 r2 r3 c1 c2 c3.
```
where `r` is row and `c` is column.

With that, if a stone is placed at row 1 (`r1`) and column 2 (`c2`), then
visually 2 options for this stone placement will be inserted into the table like
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
Note that the 2 options are horizontally linked together so they can find each
other and each option is vertically linked with its covered header so once a
header is covered, all options can be looked up quickly.

To see how it helps, the conflicting placements `(r1,c1)` and `(r1,c2)` are
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
When the placement `(r1,c1)` is selected, header `r1` will be covered and all
options it vertically linked, e.g., `(r1,c2)`, are unlinked temporarily. This
means `(r1,c2)` will not be part of the search branch anymore, until
backtracking.

