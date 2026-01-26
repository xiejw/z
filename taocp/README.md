## Boolean  Basics
|Algorithm List|
| :--- |
| V4 7.1.1C|

Horn functions (V4 7.1.1C) offer an interesting approach to logic by first
solving the definite case via a specialized waterfall algorithm, followed by an
elegant general-case solution outlined in Exercise 48. Because the Horn function
serves as a foundational logic deduction framework ($A \land B \land \dots
\Rightarrow Z$), it remains highly practical for various real-world
applications, (e.g. Consistency in Section 7.2.2.3-(88)).

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

## TODO

- Focus on Volume 4A, 4B, etc
- Write all algorithms in C++ with simple APIs.
- Use clang-19 above with c++17 only.
