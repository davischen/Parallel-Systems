# MPI-Based Barnes-Hut Simulation

## Overview
This project implements a parallelized version of the Barnes-Hut algorithm for simulating gravitational forces among particles. The implementation supports both sequential execution and MPI-based parallel execution. The program is designed for scalability across multiple processes and uses a quadtree to optimize force calculations.

---

## Features

1. **Sequential Execution:**
   - Computes gravitational forces using a Barnes-Hut quadtree.
   - Outputs particle positions and velocities after simulation steps.

2. **MPI-Based Parallel Execution:**
   - **Static Partitioning:** Divides particles evenly among MPI processes.
   - **Quadtree Construction:** Each process constructs a local quadtree and contributes to a global tree.
   - **Force Calculation:** Parallel computation of forces based on quadtree structure.
   - **Synchronization:** Uses `MPI_Allgather` or point-to-point communication for data exchange.

3. **Custom MPI Datatypes:**
   - Efficiently packs particle data for communication.

4. **Visualization Support (Optional):**
   - Enables visualizing the simulation using external tools.

---

## Files

### Main Files
- `main.cpp`: Entry point for the program. Parses command-line arguments and initializes MPI.
- `sequential.cpp`: Implements the sequential version of the Barnes-Hut algorithm.
- `parallel_mpi.cpp`: Implements the parallel version using MPI.
- `tree.cpp`: Contains functions for building and manipulating the Barnes-Hut quadtree.
- `io.cpp`: Handles reading and writing particle data from/to files.
- `argparse.cpp`: Parses command-line options.

### Header Files
- `parallel_mpi.h`: Header for MPI-based implementation.
- `tree.h`: Header for quadtree structure and functions.
- `common.h`: Common constants and utility macros.
- `particle.h`: Defines the `particle` structure.
- `argparse.h`: Header for argument parsing.

---

## How to Build

### Prerequisites
- MPI library (e.g., OpenMPI or MPICH).
- C++ compiler supporting C++11 or later.
- CMake (optional, for build automation).

### Compilation
```bash
mpic++ -o barnes_hut main.cpp sequential.cpp parallel_mpi.cpp tree.cpp io.cpp argparse.cpp -lm
```

### Running

#### Sequential Mode
```bash
./barnes_hut --input <input_file> --output <output_file> --steps <n_steps>
```

#### MPI Mode
```bash
mpirun -np <num_processes> ./barnes_hut --input <input_file> --output <output_file> --steps <n_steps> --mpi_type <a|s>
```

**Arguments:**
- `--input`: Input file containing initial particle positions and velocities.
- `--output`: Output file for final particle positions and velocities.
- `--steps`: Number of simulation steps.
- `--mpi_type`: Type of MPI communication:
  - `a`: Uses `MPI_Allgather`.
  - `s`: Uses point-to-point communication.

---

## Input File Format
- Each line represents a particle with the following fields:
  ```
  <index> <x> <y> <mass> <v_x> <v_y>
  ```

---

## Output File Format
- Each line represents a particle after the simulation with the following fields:
  ```
  <index> <x> <y> <v_x> <v_y>
  ```

---

## Key Components

### 1. **Particle Structure (`particle`)**
Defines the properties of a particle:
- `index`: Particle ID.
- `x`, `y`: Position.
- `v_x`, `v_y`: Velocity.
- `a_x`, `a_y`: Acceleration.
- `mass`: Mass of the particle.

### 2. **Barnes-Hut Quadtree (`Node`)**
- Hierarchical structure dividing 2D space into quadrants.
- Each node contains:
  - Boundaries (`min_bound`, `max_bound`).
  - Particle data.
  - Pointers to child nodes.
- Supports:
  - Insertion of particles.
  - Calculation of forces using the center-of-mass approximation.

### 3. **MPI Communication**
- Custom MPI datatype (`mpiBody`) to represent particle data.
- Two modes of communication:
  - `MPI_Allgather`: Broadcasts updated particles to all processes.
  - Send/Receive: Rank 0 collects data from other processes and redistributes it.

---

## Performance Considerations

1. **Load Balancing:**
   - Ensures even distribution of particles among processes.

2. **Communication Overhead:**
   - Reduced by minimizing message size and frequency.

3. **Tree Optimization:**
   - Limits depth of the quadtree to prevent performance degradation.

---

## Example Usage

### Sequential Execution
```bash
./barnes_hut --input data/input.txt --output data/output.txt --steps 100
```

### Parallel Execution with MPI_Allgather
```bash
mpirun -np 4 ./barnes_hut --input data/input.txt --output data/output.txt --steps 100 --mpi_type a
```

### Parallel Execution with Send/Receive
```bash
mpirun -np 4 ./barnes_hut --input data/input.txt --output data/output.txt --steps 100 --mpi_type s
```

---

## Troubleshooting

1. **MPI Initialization Errors:**
   - Ensure MPI is installed and configured correctly.
   - Verify the executable is launched using `mpirun` or `mpiexec`.

2. **Segmentation Faults:**
   - Check memory allocation and array bounds.
   - Use `valgrind` or similar tools for debugging.

3. **Incorrect Results:**
   - Validate the input file format.
   - Compare results with the sequential implementation.

---

## Future Improvements

- Add dynamic load balancing to handle uneven particle distributions.
- Extend to 3D simulations.
- Integrate visualization directly into the simulation.

---

## Authors
- **Your Name**
- **Contributors:** List contributors here

---

## License
This project is licensed under the MIT License. See the LICENSE file for details.

