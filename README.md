# RTree

## Results
- The computed time is in microseconds.

QUERY | MAX     | MIN     | AVG     | STD
------|---------|---------|---------|--------
0     | 323918  | 2040    | 43704.1 | 4012.11
1     | 541291  | 3673    | 403131  | 35262.6
2     | 608826  | 7206    | 484956  | 39159.7
3     | 148309  | 3       | 150911  | 11651
4     | 9916055 | 3290946 | 480012  | 401234

**Insertion time**
- The total time for insertion was 40 minutes.

## Observations

**B+-Tree vs R-Tree**
- Compared to B+-trees, the time taken for each of the queries is much higher. This is attributed to the smaller fanout (28) of an R-Tree as compared to the larger fanout (128) of B+-trees.
- On a similar vein, searches are slower in R-Trees compared to inserts. The opposite holds for B+-Trees due to the large fanout and smaller height.
- An R-Tree has a much larger height as compared to a B+-Tree.

**Searches vs Inserts**
- On account of the high overlap between MBR's and the high selectivity of queries, searches are much costlier than inserts.

**Fast kNNSearches**
- kNNSearch should take the same time as that of other searches, but the results obtained don't agree to theory. This is because of the small 'k' in the queries. This small k, along with the efficient best first search algorithm makes sure that the time taken by kNNSearch is much smaller than other searches which might not be true in general.

## INSTALL

- Tree parameters are defined in *[rtree.config]*(rtree.config). Make sure you run the following after such a change.

```shell
$ ./configure
```

- To build a new tree you can run:

```shell
$ make
```

- To build a fresh tree:

```shell
$ make fresh
```

- To build a tree with pre-built data:

```shell
$ make restore
```

- To time the program the configuration is as follows:

```c++
// #define OUTPUT
#define TIME
```

- To just get the output, the configuration is as follows:

```c++
#define OUTPUT
// #define TIME
```

- There are many DEBUG levels available in *[config.h]*(config.h).
