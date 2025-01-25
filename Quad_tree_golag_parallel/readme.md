# Golag for Quad Tree Implementation

## Overview
This project implements various methods to construct Binary Search Trees (BSTs), compute hashes of each tree, and compare them for equality. The program supports sequential and parallel computations using multiple workers for hashing and comparison tasks. The implementation allows for performance optimization using Goroutines and channels.

## Features
- **Tree Construction**: Generate and organize binary search trees from input data.
- **Hash Computation**: Compute unique hash values for each BST using an in-order traversal method.
- **Tree Comparison**: Compare trees for equality using sequential or parallel methods.
- **Parallelization**: Utilize Goroutines to enable multi-threaded processing for hashing and comparison.
- **Flexible Configuration**: Control the number of workers, data processing strategies, and input/output details using command-line arguments.

## Command-Line Arguments
| Flag                | Description                                                                 | Default Value |
|---------------------|-----------------------------------------------------------------------------|---------------|
| `-hash-workers`     | Number of Goroutines to compute hash values.                              | `1`           |
| `-data-workers`     | Number of Goroutines for data processing tasks.                           | `0`           |
| `-comp-workers`     | Number of Goroutines for tree comparison tasks.                           | `0`           |
| `-input`            | Path to the input file containing tree data.                              | N/A           |
| `-data-buffered`    | Use buffered channels for data-worker writes.                             | `true`        |
| `-comp-buffered`    | Use buffered work queue for comparison tasks by comparison workers.        | `true`        |
| `-cpuprofile`       | File path to save CPU profiling information.                              | None          |
| `-detailed`         | Print detailed results (hash groups and comparison groups).               | `true`        |

### Example Usage
```bash
./BST_opt -hash-workers=4 -data-workers=2 -comp-workers=2 -input=trees.txt -detailed=true
```

## Implementation Details

### Core Components
1. **Tree Structure**:
   - Represents the binary search tree with left and right child nodes.

2. **Hash Computation**:
   - Multiple methods for computing tree hashes: in-order traversal, parallelized hashing with locks, and channel-based synchronization.

3. **Tree Comparison**:
   - Sequential and parallel implementations for determining tree equality.

4. **Worker Management**:
   - Goroutines for distributing tasks and channels for synchronization.

### Parallelization Strategies
- **Hash Workers**: Compute hashes of BSTs in parallel.
- **Data Workers**: Manage updates to shared hash groups using locks or channels.
- **Comparison Workers**: Compare trees in parallel, distributing work using buffered/unbuffered channels.

## Manual Compilation
To compile the program manually, use the following command:
```bash
go build -o BST_opt main.go
```

## Input Format
The input file should contain multiple lines, where each line represents a tree. Each number in a line corresponds to a node value in the tree, separated by spaces.

### Example Input
```
5 3 7 2 4
8 6 10
1 2 3 4
```

## Output
- **Hash Groups**: Groups of trees with identical hashes.
- **Comparison Groups**: Groups of trees identified as identical based on value comparisons.
- **Execution Time**: Time taken for hashing and comparison tasks.

### Sample Output
```text
hashGroupTime: 0.002345
compareTreeTime: 0.003567
group 0: 1 3
group 1: 2 4
```

## Profiling
To enable CPU profiling, use the `-cpuprofile` flag:
```bash
./BST_opt -cpuprofile=cpu.prof
```
Analyze the profiling data using the `go tool pprof` command:
```bash
go tool pprof BST_opt cpu.prof
```
