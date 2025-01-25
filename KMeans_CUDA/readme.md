# K-Means Clustering with CPU and CUDA Implementations

This project provides multiple implementations of the K-Means clustering algorithm, focusing on performance and scalability. The implementations utilize both CPU-based sequential computation and GPU-accelerated CUDA techniques (including shared memory optimizations and Thrust-based processing). The goal is to provide a comprehensive comparison of computational methods for large-scale data clustering.

---

## Table of Contents
- [Introduction](#introduction)
- [Features](#features)
- [System Requirements](#system-requirements)
- [Installation](#installation)
  - [Using `make`](#using-make)
  - [Manual Compilation](#manual-compilation)
- [Usage](#usage)
  - [Command-Line Arguments](#command-line-arguments)
  - [Input File Format](#input-file-format)
  - [Example Commands](#example-commands)
- [Performance Analysis](#performance-analysis)
- [Implementation Details](#implementation-details)
  - [1. Sequential Implementation](#1-sequential-implementation)
  - [2. CUDA Implementation](#2-cuda-implementation)
  - [3. Thrust-Based CUDA Implementation](#3-thrust-based-cuda-implementation)
- [Debugging and Logging](#debugging-and-logging)


---

## Introduction

K-Means is one of the most popular clustering algorithms used for unsupervised learning. It partitions data points into a fixed number of clusters (`k`) by iteratively minimizing the within-cluster sum of squares. This project implements the algorithm with multiple approaches to enable performance benchmarking:
- **Sequential (CPU)**: A straightforward implementation using only CPU resources.
- **CUDA (GPU)**: A high-performance implementation using NVIDIA GPUs with shared memory optimizations.
- **Thrust-based CUDA (GPU)**: Uses NVIDIA's Thrust library to simplify parallel processing.

### Why This Project?

This project aims to:
1. Demonstrate the power of GPU-accelerated computations compared to traditional CPU methods.
2. Provide insights into optimizing memory access patterns using shared memory in CUDA.
3. Allow users to benchmark clustering performance on various datasets and configurations.

---

## Features

1. **Modular Design**:
   - Separate implementations for sequential, shared memory CUDA, and Thrust-based CUDA.
   - Easy-to-extend architecture.

2. **Performance Analysis**:
   - Tracks execution time for each stage: data transfer, kernel execution, and total runtime.
   - Breakdowns for per-iteration performance.

3. **Customizable Parameters**:
   - Users can configure the number of clusters, dimensions, iterations, convergence threshold, and input/output files.

4. **Debug and Logging**:
   - Print intermediate results (centroids, assignments) for debugging.
   - Verbose logs in debug mode.

5. **Error Handling**:
   - Comprehensive CUDA error checking ensures stability.

---

## System Requirements

- **Hardware**:
  - CUDA-capable GPU with compute capability 3.0 or higher.
  - At least 4 GB GPU memory (recommended for large datasets).

- **Software**:
  - C++ compiler supporting C++17.
  - CUDA Toolkit 10.1 or higher.
  - Thrust library (included with CUDA).
  - `make` utility (optional).

---

## Installation

### Using `make`
A `Makefile` is included for easy compilation. Run:
```bash
make
```
This will generate three executables:
- `kmeans_sequential`
- `kmeans_cuda`
- `kmeans_thrust`

### Manual Compilation
To compile individual implementations:
- **Sequential Implementation**:
  ```bash
  g++ -std=c++17 -o kmeans_sequential k_means_sequential.cpp -I.
  ```
- **CUDA Implementation**:
  ```bash
  nvcc -o kmeans_cuda k_means_cuda.cu -I.
  ```
- **Thrust-Based CUDA Implementation**:
  ```bash
  nvcc -o kmeans_thrust k_means_thrust.cu -I.
  ```

---

## Usage

### Command-Line Arguments

| Argument          | Description                              | Default Value |
|-------------------|------------------------------------------|---------------|
| `--n_clusters`    | Number of clusters                      | `2`           |
| `--dimensions`    | Number of dimensions per data point      | `2`           |
| `--in_file`       | Input file containing data points        | Required      |
| `--max_iterations`| Maximum number of iterations            | `100`         |
| `--threshold`     | Convergence threshold                   | `0.0001`      |
| `--seed`          | Random seed for initialization          | `12345`       |
| `--print_centroids`| Print final centroids (flag)            | Off           |
| `--debug`         | Enable debug mode (flag)                | Off           |

### Input File Format

The input file should contain data points in the following format:
```
x1 y1 z1
x2 y2 z2
...
xn yn zn
```
Where each line corresponds to a data point in `dimensions` space.

### Example Commands

- Run the **sequential implementation**:
  ```bash
  ./kmeans_sequential --n_clusters 3 --dimensions 2 --in_file data.txt --max_iterations 100
  ```

- Run the **CUDA shared memory implementation**:
  ```bash
  ./kmeans_cuda --n_clusters 3 --dimensions 2 --in_file data.txt --max_iterations 100 --threshold 0.001
  ```

- Run the **Thrust-based CUDA implementation**:
  ```bash
  ./kmeans_thrust --n_clusters 4 --dimensions 3 --in_file data_large.txt --debug
  ```

---

## Performance Analysis

Performance metrics include:
1. **Execution Time**: Total runtime for clustering.
2. **Transfer Time**: Time spent transferring data between host and device.
3. **Kernel Time**: Time spent on GPU computations.

### Sample Results

| Implementation      | Data Points | Dimensions | Clusters | Time (ms) |
|---------------------|-------------|------------|----------|-----------|
| Sequential          | 10,000      | 2          | 3        | 1200      |
| CUDA (Shared Memory)| 10,000      | 2          | 3        | 150       |
| Thrust              | 10,000      | 2          | 3        | 200       |

---

## Implementation Details

### 1. Sequential Implementation
The simplest implementation using basic loops for distance calculation and cluster assignment. Suitable for small datasets but slow for large-scale clustering.

### 2. CUDA Implementation
Optimized GPU implementation using shared memory to reduce global memory accesses. This implementation is significantly faster for large datasets.

### 3. Thrust-Based CUDA Implementation
Uses NVIDIA's Thrust library for easier implementation of parallel algorithms like reduction and transformation. It simplifies code but may have slightly lower performance than direct CUDA.

---

## Debugging and Logging

Enable debug mode with the `--debug` flag to:
- Print initial centroids.
- Show intermediate cluster assignments.
- Log convergence progress.

Example:
```bash
./kmeans_cuda --n_clusters 3 --dimensions 2 --in_file data.txt --max_iterations 100 --debug
```
