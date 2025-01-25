#include "prefix_sum.h"
#include "helpers.h"
#include "spin_barrier.h"
//#include "pthread_barrier.h"

void* compute_prefix_sum(void *a)
{
    //prefix_sum_args_t *args = (prefix_sum_args_t *)a;

    /************************
     * Your code here...    *
     * or wherever you like *
     ************************/
    // Extract the arguments from the args struct
    prefix_sum_args_t *args = (prefix_sum_args_t *)a;
    // Extract the variables from args for readability
    int n_vals = args->n_vals;  // Total number of data elements
    int n_threads = args->n_threads;  // Total number of threads
    int t_id = args->t_id;  // Thread ID determining which data block the thread processes
    int (*op)(int, int, int) = args->op;  // Function pointer to define how two elements are merged
    int n_loops = args->n_loops;  // Additional parameter used in the operation
    int *input_vals = args->input_vals;  // Input data array
    int *output_vals = args->output_vals;  // Array where results are stored

    int max_depth = 0;
    
    /*
     * UpSweep phase:
     * Start from depth = 1, doubling the depth each iteration (depth <<= 1), until max_depth is reached.
     */
    //int max_depth = ceil(log2(args->n_vals));
    int logSize = logTwo(args->n_vals)+1;
    //std::cout << "Thread " << t_id << "  logSize=" << logSize << " n_vals=" << n_vals << " starting UpSweep phase" << std::endl;
    //for d from 0 to (lg n) − 1
    for (int depth = 0; depth < logSize; depth++) {
        max_depth = depth;
        int stride_size = (1 << (depth+1));  // stride_size = 2^(depth+1) // stride_size is twice the current depth
        // partition size for a thread
        int num_chunks = n_vals / stride_size;
        int chunk_size = (num_chunks / n_threads) +
            ((num_chunks % n_threads) == 0 ? 0 : 1);
        // std::cerr << chunk_size << std::endl;
        chunk_size = std::max(1, chunk_size); 
        //int block_start = t_id * chunk_size*stride_size ;
        int valid_end = std::min( (t_id+1) * chunk_size * stride_size, n_vals);
        for (int i = t_id * chunk_size*stride_size; i < valid_end; i += stride_size) {
            int dest_index = i + stride_size - 1;  //i+2^(depth+1)-1, dest_index is the target index, stride_size=
            int prev_index = i + (stride_size >> 1) - 1; //i+2^(depth)-1, prev_index is the previous index used in the addition
            if (dest_index >= n_vals || prev_index >= n_vals) {
                continue;  // 跳過不合法的索引
            }
            // Ensure correct initialization for the first depth
            //if (depth == 1) {
            if ((1 << depth) == 1){
                output_vals[prev_index] = input_vals[prev_index];
                if (dest_index < n_vals) {
                    output_vals[dest_index] = input_vals[dest_index];
                }
            }
            if (dest_index < n_vals) {
                // Merge and store the result
                output_vals[dest_index] = op(output_vals[dest_index], output_vals[prev_index], n_loops);
                
                // Print values after the operation
                //std::cout << "Thread " << t_id << " - UpSweep (after merge): output_vals[" << dest_index << "]=" << output_vals[dest_index] << std::endl;
            }
        }
        // Synchronize threads
        // Wait on the appropriate barrier (spin or pthread)
        //std::cout << "Waiting... " << args->spin << std::endl;
        if (args->spin) {
            //static_cast<spin_barrier*>(args->barrier)->wait();
            ((spin_barrier*)args->barrier)->wait();
        } else {
            pthread_barrier_wait(static_cast<pthread_barrier_t*>(args->barrier));
        }
    }
    
    
    /*
     * DownSweep phase:
     * Start from max_depth and halve the depth each iteration (depth >>= 1), until depth = 1.
     */
    //std::cout << "Thread " << t_id << " starting DownSweep phase" << std::endl;
    //for d from (lg n) − 1 downto 0
    for (int depth = max_depth; depth >= 0; depth--) { 
        int stride_size = (1<<depth) * 2;  //2^(depth+1), stride_size is twice the current depth

        //std::cerr << "DownSweep stride_size=" << stride_size << ", depth=" << depth << std::endl;
        // partition size for a thread
        int num_blocks = n_vals / stride_size;
        int chunk_size = (num_blocks / n_threads) +
            ((num_blocks % n_threads) == 0 ? 0 : 1);
        chunk_size = std::max(1, chunk_size);
        //int block_start = t_id * chunk_size*stride_size ;
        int valid_end = std::min((t_id + 1) * chunk_size * stride_size, n_vals);
        for (int i = t_id * chunk_size*stride_size;  i < valid_end; i += stride_size) {
            int reduced_index = (i + stride_size) - 1; //i+2^(depth+1)-1
            int dest_index = reduced_index + (1<<depth); //(i + stride_size + (1<<depth))-1; //i+2^(depth)-1
        
            if (dest_index < args->n_vals) {
                    
                    output_vals[dest_index] = op(output_vals[dest_index], output_vals[reduced_index], n_loops);
                
                    // Print values after the operation
                    //std::cout << "Thread " << t_id << " - DownSweep (after merge): output_vals[" << dest_index << "]=" << output_vals[dest_index] << std::endl;
            }
        }
        // Synchronize threads
        // Use the appropriate barrier type
        // Wait on the appropriate barrier (spin or pthread)
        if (args->spin) {
            //static_cast<spin_barrier*>(args->barrier)->wait();
            ((spin_barrier*)args->barrier)->wait();
        } else {
            pthread_barrier_wait(static_cast<pthread_barrier_t*>(args->barrier));
        }
    }

    return 0;
}