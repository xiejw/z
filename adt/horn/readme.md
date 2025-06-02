### TAOCP v4 Horn Clause and Formula

#### TLDR

Concludes whether horn formula is satisfiable.

#### Definition

A Horn clause is a clause (a disjunction of literals) with at most one
positive literal. A Horn formula is a propositional formula formed by
conjunction of Horn clauses.

#### Definite Horn Formula and The Core

The definite Horn formula must satisfy

```
f(1, 1, ..., 1) = 1.
```

The _Core_ of the definite Horn formula is set the variables which must be true
whenever `f` is true.

#### Algorithrm C

The algorithm is as follows:

- Put all positive literal in the single variable clause into Core.  They
  must be true.

- Keep deducing the proposition of non-positive literal, once their values
  are known to be true, i.e., in Core. Once all non-positive literals in a
  clause are deduced, its positive literal, if present, must be in Core.

#### Indefinite Horn Formula

_Exercise 48_ provides the steps to test satisfiability of Horn formula in
general. The idea is quite simple:

- Introduce a new variable `lambda`, and convert all indefinite causes to
  definite cause. For example, the following indefinite Horn clause

  ```
  !a || !b
  ```

  will be converted as

  ```
  !a || !b || lambda
  ```

- Apply _Algorithrm C_ to the new definite Horn formula. The original Horn
  formula is satisfiable if and only if `lambda` is not in the Core of the
  new definite Horn formula.


#### Sample Code

See [`cmd/horn/main.c`](cmd/horn/main.c). To play with it, try
```
make test
make horn
```
