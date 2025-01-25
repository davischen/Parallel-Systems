#include <iostream>
#include <argparse.h>
#include <threads.h>
#include <io.h>
#include <chrono>
#include <cstring>
#include "operators.h"
#include "helpers.h"
#include "prefix_sum.h"
//#include "pthread_barrier.h"

using namespace std;

int main(int argc, char **argv)
{
    // Parse args
    struct options_t opts;
    get_opts(argc, argv, &opts);

    bool sequential = false;
    if (opts.n_threads == 0) {
        opts.n_threads = 1;
        sequential = true;
    }

    // Setup threads
    pthread_t *threads = sequential ? NULL : alloc_threads(opts.n_threads);;
    if (!sequential && threads == NULL) {
        std::cerr << "Error allocating memory for threads" << std::endl;
        exit(1);
    }
    
    // Setup args & read input data
    prefix_sum_args_t *ps_args = alloc_args(opts.n_threads);
    int n_vals;
    int *input_vals, *output_vals;
    read_file(&opts, &n_vals, &input_vals, &output_vals);

    if (input_vals == NULL || output_vals == NULL) {
        std::cerr << "Error allocating memory for input/output arrays" << std::endl;
        free(ps_args);
        free(threads);
        exit(1);
    }
    
    
    //"op" is the operator you have to use, but you can use "add" to test
    int (*scan_operator)(int, int, int);
    scan_operator = op;
    //scan_operator = add;

    
    //Initialize the appropriate barrier based on ps_args.spin
    //opts.spin = true;
    
    //spin_barrier barrier(opts.n_threads);
    // Use spin barrier
    void *barrier;
    if (opts.spin) {
        // Initialize the spin barrier
        //std::cout << "Initialize spin barrier " << opts.spin << std::endl;
        barrier = new spin_barrier(opts.n_threads);
    } else {
        // Initialize the pthread barrier
        barrier = new pthread_barrier_t();  // Properly allocate memory for the pthread barrier
        //std::cout << "Initialize pthread barrier " << opts.spin << std::endl;
        pthread_barrier_init(static_cast<pthread_barrier_t*>(barrier), NULL, opts.n_threads);
    }

    // Call fill_args with the barrier as a void*
    fill_args(ps_args, opts.n_threads, n_vals, input_vals, output_vals,
            opts.spin, op, opts.n_loops, barrier);  // No need to cast here

    // Start timer
    auto start = std::chrono::high_resolution_clock::now();

    if (sequential)  {
        //sequential prefix scan
        output_vals[0] = input_vals[0];
        for (int i = 1; i < n_vals; ++i) {
            //y_i = y_{i-1}  <op>  x_i
            output_vals[i] = scan_operator(output_vals[i-1], input_vals[i], ps_args->n_loops);
        }
    }
    else {
        //start_threads(threads, opts.n_threads, ps_args, <your function>);
        start_threads(threads, opts.n_threads, ps_args, compute_prefix_sum);

        // Wait for threads to finish
        join_threads(threads, opts.n_threads);
    }

    //End timer and print out elapsed
    auto end = std::chrono::high_resolution_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "time: " << diff.count() << std::endl;

    // Write output data
    write_file(&opts, &(ps_args[0]));
    // Free allocated memory, ensuring each block is freed only once
    // Cleanup: Destroy the appropriate barrier
    // Cleanup barriers
    // Cleanup the barrier
    if (opts.spin) {
        delete static_cast<spin_barrier*>(barrier);  // Cast back to spin_barrier and delete
    } else {
        pthread_barrier_destroy(static_cast<pthread_barrier_t*>(barrier));  // Cast and destroy pthread barrier
        delete static_cast<pthread_barrier_t*>(barrier);  // 使用 delete 而不是 free
    }
    
    if (threads != NULL) {
        free(threads);
        threads = NULL;
    }

    if (ps_args != NULL) {
        free(ps_args);
        ps_args = NULL;
    }
}