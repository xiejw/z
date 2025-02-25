## SAT Solver

The solver expects `n` boolean variables and tries to identify whether the `m`
clauses are satisfiable. User is responsible to translate the application domain
problem into SAT problem first and map the result back to the solution to the
original problem.

### Use Case

#### The Problem

Let's take the [Dependency Resolution
problem](https://borretti.me/article/dependency-resolution-made-simple) as a use
case study. Assume we have the following package database

| Package  | Versions      |
| -------- | --------      |
| app      | 0             |
| sql      | 0, 1, 2       |
| threads  | 0, 1, 2       |
| http     | 0, 1, 2, 3, 4 |
| stdlib   | 0, 1, 2, 3, 4 |

And we want to install `app:0`, which has dependencies `sql`, `threads`, `http`,
and `stdlib`. Each dependency, as a package, might have its own dependencies;
and dependencies are different for different versions.

For this case study, we also assume the dependency is recorded
with a minimum version and a maximum version. For example,
```
app:0
  -  sql     0,  2
  -  threads 1,  1
```
means `app:0` needs two dependencies `sql` and `threads`. Any version `0, 1, 2`
of `sql` is OK, but only version `1` of `threads` is allowed for `app:0`.

With that, the entire dependency tree is as follows
```
             min,max
app:0
  -  sql     2,  2
  -  threads 2,  2
  -  http    3,  4
  -  stdlib  4,  4
sql:0
sql:1
  -  stdlib  1,  4
  -  threads 1,  1
sql:2
  -  stdlib  2,  4
  -  threads 1,  2
threads:0
  -  stdlib  2,  4
threads:1
  -  stdlib  2,  4
threads:2
  -  stdlib  3,  4
http:0
  -  stdlib  0,  3
http:1
  -  stdlib  0,  3
http:2
  -  stdlib  1,  4
http:3
  -  stdlib  2,  4
http:4
  -  stdlib  3,  4
```

#### Translation

**Package Versions are Variables**:
We assign one boolean variable for each `package:version` pair, where `true`
means this `package:version` is installed and `false` means not installed.

**Version Ranges are Disjunctions**:
For `app:0` and one of its dependencies `http` (`min:3, max:4`), we can write a
disjunction of all allowed versions, i.e.,
```
http:3 or http:4
```
Then the fact either of them is installed (`true`) means the disjunction is
`true`.

**Dependencies are Implications**:
For `app:0` and one of its dependencies `http`, we express the dependency as
```
app:0 => http:3 or http:4
```
which is equivalent to
```
not(app:0) or http:3 or http:4
```

**Consistency**:
For `sql` with three versions `0, 1, 2`, only one version is allowed to be
installed, which can be expressed as
```
(not(sql:0 and sql:1)) and
(not(sql:0 and sql:2)) and
(not(sql:1 and sql:2))
```
which is same as
```
(not(sql:0) or not(sql:1)) and
(not(sql:0) or not(sql:2)) and
(not(sql:1) or not(sql:2))
```

**The Root of the DAG**: The final SAT problem is the conjunction of all terms
above with `app:0` as `true` (the root).

### Algorithm B: Satisfiability by Watching
