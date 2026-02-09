## Boolean  Basics
|Algorithm List|
| :--- |
| V4 7.1.1C|
| V4 7.1.1 Theorem K|

Horn functions (V4 7.1.1C) offer an interesting approach to logic by first
solving the definite case via a specialized waterfall algorithm, followed by an
elegant general-case solution outlined in Exercise 48. Because the Horn function
serves as a foundational logic deduction framework ($A \land B \land \dots
\Rightarrow Z$), it remains highly practical for various real-world
applications, (e.g. Consistency in Section 7.2.2.3-(88)).

2-SAT is a conjunction of Krom clauses. As a special case, its satisfiability
can be determined in almost linear time, as shown by Theorem K in Section V4,
7.1.1 and Exercise 54 (p. 86). This provides an excellent example of how
graph-theoretic concepts—specifically strongly connected components—can be
applied to solve challenging problems with linear-time complexity.

## Backtrack Algorithm

|Algorithm List|
| :--- |
| V4 7.2.2.B |
| V4 7.2.2.W |

The key to designing an effective backtracking algorithm lies in three main aspects:
- A. a well-designed data structure,
- B. an efficient undo mechanism, and
- C. highly aggressive pruning heuristics to cut off the search space early.

### N=16 Queues

| Algorithm | Mems | Wall Clock (Apple M4 Max) |
| ------------- | ------------- | --|
| V4 7.2.2B Basic backtrack  | 33'859'294'165  | 40 secs |
| V4 7.2.2B Basic backtrack+Bit vector  | 2'282'380'604  | 33 secs|
| V4 7.2.2W Walker's backtrack  | 6'893'407'587  | 4.7 secs|

## Components and Traversal

|Algorithm List|
| :--- |
| V4 7.4.1.2.T |

In Pre-Fascicle 12A, the elegant strongly connected components algorithm is
examined in detail, including its application to 2-SAT (Boolean satisfiability).
It perfectly exemplifies an algorithm that achieves "intelligence" not through
unnecessary complexity, but through the strategic placement of simple data.

## TODO

- Focus on Volume 4A, 4B, etc
- Write all algorithms in C++ with simple APIs.
- Use clang-19 above with c++17 only.

- **Dancing Link** ([dlink](./taocp/v4_dlink)): An advanced data structure
  implementing backtracking algorithms, for the exact cover problem (XC). This
  can be extended to help exact cover with colors problem (XCC).
- **SAT Solver** ([sat](./taocp/v4_sat)): Few algorithms which aim to solve the
  Boolean satisfiability problem (SAT).
