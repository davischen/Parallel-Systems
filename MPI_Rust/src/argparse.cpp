#include "argparse.h"
#include <iostream>
#include <cstdlib>
#include <getopt.h>
#include <string>
#include "common.h"
#include <climits>  // 添加這行以定義 INT_MAX

void get_opts(int argc, char** argv, options_t* opts) {
    // 初始化預設參數
    opts->in_file = "";
    opts->out_file = "";
    opts->n_steps = 100;
    opts->threshold = 0.5;
    opts->timestep = 0.01;
    opts->n_particles = 100; // 預設粒子數量
    opts->visualization = false;
    opts->sequential = false;
    opts->mpi_type = ""; // 預設 MPI 類型

    const struct option long_options[] = {
        {"input", required_argument, 0, 'i'},
        {"output", required_argument, 0, 'o'},
        {"steps", required_argument, 0, 's'},       // 改為 's'
        {"threshold", required_argument, 0, 't'},
        {"timestep", required_argument, 0, 'd'},
        {"bodies", required_argument, 0, 'b'},
        {"visualization", no_argument, 0, 'V'},
        {"sequential", no_argument, 0, 'S'},       // 改為 'S'
        {"mpi_type", required_argument, 0, 'm'},   // 添加 mpi_type
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "i:o:s:t:d:b:VSm:", long_options, nullptr)) != -1) { // 'n' -> 's', 's' -> 'S'
        //DEBUG_PRINT(std::cout << "Parsing option: " << (char)opt << ", argument: " << (optarg ? optarg : "null") << std::endl); // 調試輸出
        switch (opt) {
            case 'i': opts->in_file = std::string(optarg); break;
            case 'o': opts->out_file = std::string(optarg); break;
            case 's': opts->n_steps = std::stoi(optarg); break;   // 更新為 's' 對應 `n_steps`
            case 't': opts->threshold = std::stod(optarg); break;
            case 'd': opts->timestep = std::stod(optarg); break;
            case 'b': opts->n_particles = std::stoi(optarg); break;
            case 'V': opts->visualization = true; break;
            case 'S': opts->sequential = true; break;            // 更新為 'S' 對應 `sequential`
            case 'm': opts->mpi_type = std::string(optarg); break; // 設定 MPI 類型
            default:
                std::cerr << "Invalid option. Use --help for usage information.\n";
                exit(EXIT_FAILURE);
        }
    }

    // 驗證參數
    if (opts->n_particles <= 0 || opts->n_particles > INT_MAX)
    {
        std::cerr << "Invalid number of inputs in input file " << std::endl;
        exit(EXIT_FAILURE);
    }
    if (opts->in_file.empty()) {
        std::cerr << "Error: Input file not specified!" << std::endl;
        exit(EXIT_FAILURE);
    }
    if (opts->out_file.empty()) {
        std::cerr << "Error: Output file not specified!" << std::endl;
        exit(EXIT_FAILURE);
    }
}
