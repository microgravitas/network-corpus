## Network corpus

A collection of networks from various online repositories in a simple file format
for algorithmic benchmarks. Citation information and links to the sources are, where 
available, listed in the respective `.info` file.

### Network format

Each network is stored as a simple gzipped edge-list, nodes are represented by
integers. Nodes of degree zero have been removed.

### Network statistics

The file `statistics.json` contains various statistics about the networks. This
is subject to change and I recommend not using it at this stage.
