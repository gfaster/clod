# Cluster Based Dynamic Level of Detail

This was my final project submission for Harvard CSCI E-72, Introduction to
Computer Graphics. A good portion the OpenGL rendering code was provided as
template code but object loading and the simplification work was written by
myself.

A demonstration can be found at [this YouTube video][demo]

The project is a basic implementation of Unreal Engine 5's Nanite level of
detail system, which is detailed in [this talk][talk].

## Technique

This level of detail method provides a way to render multiple different LODs
simultaneously on the same object without any cracks forming. The steps to do so
are as follows:

1. Partition the mesh into triangle clusters, minimizing the edge-cut. I use a
   cluster size of 64.

2. Partition the clusters into groups of no more than 4, minimizing edge-cut.
   The weights of the edges are the number of triangle edges shared between the
   two clusters.

3. Within the groups, perform edge collapse while keeping edges on the
   boundary between clusters locked. I use a simple minimum edge length metric
   over the superior [quadric error metric][qem].

4. Within the simplified groups, repartition the triangles into new clusters.

5. Repeat steps 2 through 4 until sufficiently simplified.

The result is a DAG that represents hierarchical level of detail without
introducing high-detail boundaries between subtrees that would form in a more
naive algorithm.

## Running it
Currently, due to an unknown reason when moving to using the Debian
repositories, running always results in a crash related to METIS.

Requires libOpenGL, libSDL2, libMETIS, g++, and make. This was last tested on
Debian 12.

```bash
make clod
```




[demo]: https://www.youtube.com/watch?v=-wF9icVke58
[talk]: https://www.youtube.com/watch?v=eviSykqSUUw
[qem]: https://dl.acm.org/doi/10.1145/258734.258849
