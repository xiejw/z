## AIC

Artificial Intelligence Compute or AI Infrastrutre Compute.

### Staring Project
This is a very simple pure c code to run llama 3 8B model.

During the coding, I find it is fascinating to learn so many new algorithm at
the coding level, which I only briefly read the papers.

### Algorithms

#### Tokenizer

**Algorithm** This [blog post][1] provides a good introduction to the BPE
Algorithm with testing code. The only missing piece is the special tokens
handling, which is trivial to add.

**Implementation** For the C implementation, there are few challenges
- _How to support word splitting regexp?_ I debate between manual state machine
  implementation and `PCRE2` library. The `llama.cpp` project uses former with
  quite a ton code to handle unicode. In the first implementation, I leverage
  `PCRE2` which means an external dependency is introduced.
- _How to avoid small memory allocations?_ Based on the research of many code
  bases, almost all implementations allocates strings after word splitting and
  during BPE merging. I guess these are due to the fact many high level
  programming languages encourage users to use strings as basic data types not a
  pointer with byte array lengths and do not provide low level primitives to
  support hashing etc. My implementation is almost zero allocation.
- _How to speed up the hash table lookup?_ To build a python mergeable rank hash
  table, it takes 57ms on my laptop. Any good c implementation can be cut under
  6ms. This largely affects the BPE algorithm as well. Again, most high level
  programming languages provide _slow_ hashing functions which are targeted
  different use cases. I used a simple algorithm and monitored the conflicting
  rates. It serves the purpose very well. In future, SIMD could be used to
  further optimize the performance if needed.

[1]: https://eli.thegreenplace.net/2024/tokens-for-llms-byte-pair-encoding-in-go/
