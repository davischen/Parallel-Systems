#include <iostream>
#include <cstdlib>
#include "argparse.h"
#include "sequential.h"
#include "parallel_mpi.h"
#include <mpi.h>
//#include "visualization.cpp"

int main(int argc, char** argv) {
    
    options_t options;

    // 解析命令行參數
    get_opts(argc, argv, &options);

/*#ifdef DEBUG
    DEBUG_PRINT(std::cout << "Input file: " << options.in_file << std::endl);
    DEBUG_PRINT(std::cout << "Output file: " << options.out_file << std::endl);
    DEBUG_PRINT(std::cout << "Number of steps: " << options.n_steps << std::endl);
    DEBUG_PRINT(std::cout << "Threshold: " << options.threshold << std::endl);
    DEBUG_PRINT(std::cout << "Timestep: " << options.timestep << std::endl);
    DEBUG_PRINT(std::cout << "Visualization: " << (options.visualization ? "Enabled" : "Disabled") << std::endl);
    DEBUG_PRINT(std::cout << "Sequential Mode: " << (options.sequential ? "Enabled" : "Disabled") << std::endl);
    DEBUG_PRINT(std::cout << "---------------------------------" << std::endl);
#endif*/
    int size, rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    /*if(options.visualization){
            if (init_visualization() != 0) {
                fprintf(stderr, "Failed to initialize visualization.\n");
                return -1;
            }
    }*/
    if (size <= 1) {
        // 將 `options` 傳遞為指標
        //DEBUG_PRINT(std::cout << "Sequential version" << std::endl);
        return BHSeq(&options); // 返回 `BHSeq` 的結果
    } else {
        DEBUG_PRINT(std::cout << "MPI version" << std::endl);
        
        //std::cout << "Size:" << size << std::endl;
        // 將 `options` 傳遞為指標
        if(options.mpi_type=="")
        {
          return parallel_mpi_send_recv(&options, rank, size); // 呼叫 `barnes_hut_mpi`，它的返回值是 void
        }
        if(options.mpi_type=="a")
        {
          return parallel_mpi(&options, rank, size); // 呼叫 `barnes_hut_mpi`，它的返回值是 void
        }
        if(options.mpi_type=="s")
        {
          return parallel_mpi_send_recv(&options, rank, size); // 呼叫 `barnes_hut_mpi`，它的返回值是 void
        }

    }

    return EXIT_SUCCESS;
}
