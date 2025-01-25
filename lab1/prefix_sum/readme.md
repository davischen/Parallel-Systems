# Parallel Prefix Sum Implementation

This program implements a parallel prefix sum algorithm using both spin barriers and `pthread` barriers for synchronization between threads. It is designed to compute prefix sums efficiently on large datasets, leveraging multi-threading and synchronization mechanisms. The program supports both sequential and parallel execution modes, depending on the input configuration.

## Features

- **Prefix Sum Computation**: Computes prefix sums (`scan operation`) on an input dataset.
- **Parallel and Sequential Modes**: Allows execution with multiple threads or a single thread for sequential computation.
- **Thread Synchronization**:
  - Uses `pthread_barrier_t` for thread synchronization.
  - Supports spin barriers implemented via a custom `spin_barrier` class.
- **Customizable Operations**: Allows defining custom operations (`op`) for the prefix sum calculation.
- **Command-Line Arguments**: Supports input/output file specification, thread count, number of loops, and synchronization type (spin or pthread).

## File Structure

- **`main.cpp`**: Contains the main program logic for argument parsing, initialization, thread management, and execution.
- **`prefix_sum.cpp`**: Implements the prefix sum computation, including the `UpSweep` and `DownSweep` phases.
- **`spin_barrier.cpp`**: Implements the spin barrier for thread synchronization.
- **`threads.cpp`**: Provides utilities for thread creation and joining.
- **`helpers.cpp`**: Utility functions for reading and writing data, argument parsing, and memory allocation.

## Usage

### Command-Line Arguments

| Argument              | Description                                                                 |
|-----------------------|-----------------------------------------------------------------------------|
| `--in` or `-i`        | Path to the input file containing the dataset.                             |
| `--out` or `-o`       | Path to the output file for saving results.                                |
| `--n_threads` or `-n` | Number of threads to use. Set to `0` for sequential execution.             |
| `--loops` or `-l`     | Number of iterations to perform for the custom operation.                  |
| `--spin` or `-s`      | Use a spin barrier instead of a `pthread_barrier_t` for thread synchronization (optional). |

### Example Command

```bash
./prefix_sum -i input.txt -o output.txt -n 4 -l 100 -s
