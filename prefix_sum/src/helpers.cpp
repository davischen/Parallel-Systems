#include "helpers.h"

prefix_sum_args_t* alloc_args(int n_threads) {
  return (prefix_sum_args_t*) malloc(n_threads * sizeof(prefix_sum_args_t));
}

int next_power_of_two(int x) {
    int pow = 1;
    while (pow < x) {
        pow *= 2;
    }
    return pow;
}

unsigned int logTwo(unsigned int x)
{
    int result = 0;
    while(x >>= 1) result++;
    return result;
}

void fill_args(prefix_sum_args_t *args,
               int n_threads,
               int n_vals,
               int *inputs,
               int *outputs,
               bool spin,
               int (*op)(int, int, int),
               int n_loops,
               void *barrier) {
    for (int i = 0; i < n_threads; ++i) {
        //args[i] = {inputs, outputs, spin, n_vals,
        //           n_threads, i, op, n_loops, barrier};
         args[i].input_vals = inputs;
        args[i].output_vals = outputs;
        args[i].n_vals = n_vals;
        args[i].n_threads = n_threads;
        args[i].t_id = i;
        args[i].op = op;
        args[i].n_loops = n_loops;

        // Store the barrier based on its type
        args[i].barrier = barrier;
        args[i].spin = spin;  // Store spin flag to know which barrier to use
    }
}