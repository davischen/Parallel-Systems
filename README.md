# Parallel-Systems Project Overviews

This repository includes detailed implementations of five computational projects that explore high-performance computing, parallel processing, and distributed systems. Each project demonstrates distinct methodologies to solve complex problems efficiently and effectively.

---

## 1. K-Means Clustering with CPU and CUDA Implementations

### Overview
This project provides a comprehensive implementation of the K-Means clustering algorithm using CPU-based sequential computation and GPU-accelerated CUDA techniques. The focus is on performance optimization and scalability for large datasets.

### Features
- Sequential CPU implementation.
- CUDA implementation with shared memory optimization.
- Thrust-based CUDA processing.
- Performance benchmarking and detailed execution analysis.
- Configurable parameters for clusters, iterations, and thresholds.

### Details
This project explores how shared memory and Thrust simplify and accelerate K-Means computation. Execution times and memory usage are benchmarked across CPU and GPU implementations to highlight performance gains.

---

## 2. MPI-Based Barnes-Hut Simulation

### Overview
Implements the Barnes-Hut algorithm to simulate gravitational forces among particles. Supports both sequential and MPI-based parallel execution using a quadtree for efficient force calculations.

### Features
- Sequential and MPI-based parallel implementations.
- Custom MPI datatypes for particle communication.
- Quadtree construction for efficient force approximation.
- Dynamic load balancing and synchronization mechanisms.

### Details
The project demonstrates scalability across multiple processes using MPI. A comparison between `MPI_Allgather` and point-to-point communication modes provides insights into communication overhead and efficiency.

---

## 3. Parallel Prefix Sum Implementation

### Overview
Implements a work-efficient parallel prefix sum algorithm using pthread barriers and spin barriers. Designed to compute prefix sums for large datasets efficiently.

### Features
- UpSweep and DownSweep phases for prefix sum computation.
- Supports customizable thread counts and synchronization methods.
- Seamless integration of spin barriers and pthread barriers.

### Details
This project focuses on optimizing thread synchronization and load balancing, demonstrating the impact of different barrier mechanisms on performance.

---

## 4. Two-Phase Commit (2PC) Protocol Implementation

### Overview
A Rust-based implementation of the Two-Phase Commit (2PC) protocol to ensure distributed consistency in transactional systems.

### Features
- Coordinator, participant, and client modules.
- Timeout-based fault tolerance.
- Operation logging for debugging and recovery.
- Flexible configuration for probabilities of success and failure.

### Details
The project highlights how the 2PC protocol ensures consistency across distributed systems while addressing communication reliability and fault tolerance.

---

## 5. Golang-Based Quadtree Implementation

### Overview
Implements various methods to construct Binary Search Trees (BSTs), compute hashes, and compare them for equality. This project uses Goroutines and channels to optimize performance.

### Features
- Parallel tree construction and hashing.
- Efficient use of Goroutines and buffered channels.
- Profiling support with detailed performance metrics.

### Details
Focuses on leveraging Go's concurrency model to handle large-scale BST operations, demonstrating the power of Goroutines and channels for parallel computing.

