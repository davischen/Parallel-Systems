#include "argparse.h"
#include <getopt.h>
#include <iostream>
#include <cstdlib>

void parse_args(int argc, char **argv, options_t &opts) {
    // 初始化默認參數
    opts.n_clusters = 2;
    opts.dimensions = 2;
    opts.max_iterations = 100;
    opts.threshold = 0.0001;
    opts.seed = 12345;
    opts.print_centroids = false;
    opts.debug = false;  // 默認情況下關閉 debug 模式
    opts.algorithm = 0;

    // 定義命令列選項
    struct option long_options[] = {
        {"n_clusters", required_argument, NULL, 'k'},
        {"dimensions", required_argument, NULL, 'd'},
        {"in_file", required_argument, NULL, 'i'},
        {"max_iterations", required_argument, NULL, 'm'},
        {"threshold", required_argument, NULL, 't'},
        {"print_centroids", no_argument, NULL, 'c'},
        {"seed", required_argument, NULL, 's'},
        {"algorithm", required_argument, NULL, 'a'},
        {"output_file", no_argument, NULL, 'o'},  // 新增 output_file 選項
        {"debug", no_argument, NULL, 'g'},  // 新增 debug 選項
        {0, 0, 0, 0}
    };

    int option_index = 0;
    while (1) {
        int c = getopt_long(argc, argv, "k:d:i:m:t:cs:a:o:g", long_options, &option_index);
        if (c == -1) break;
        switch (c) {
            case 'k':
                opts.n_clusters = std::atoi(optarg);
                break;
            case 'd':
                opts.dimensions = std::atoi(optarg);
                break;
            case 'i':
                opts.in_file = optarg;
                break;
            case 'm':
                opts.max_iterations = std::atoi(optarg);
                break;
            case 't':
                opts.threshold = std::atof(optarg);
                break;
            case 'c':
                opts.print_centroids = true;
                break;
            case 's':
                opts.seed = std::atoi(optarg);
                break;
            case 'a':
                opts.algorithm = std::atoi(optarg);
                break;
            case 'o':
                opts.output_file = optarg;  // 設置 output_file
                break;
            case 'g':
                opts.debug = true;  // 開啟 debug 模式
                break;
            default:
                std::cerr << "Unknown option." << std::endl;
                exit(EXIT_FAILURE);
        }
    }

    // Debug 模式輸出參數
    if (opts.debug) {
        std::cout << "Debug mode enabled:" << std::endl;
        std::cout << "n_clusters: " << opts.n_clusters << std::endl;
        std::cout << "dimensions: " << opts.dimensions << std::endl;
        std::cout << "in_file: " << opts.in_file << std::endl;
        std::cout << "max_iterations: " << opts.max_iterations << std::endl;
        std::cout << "threshold: " << opts.threshold << std::endl;
        std::cout << "print_centroids: " << (opts.print_centroids ? "true" : "false") << std::endl;
        std::cout << "seed: " << opts.seed << std::endl;
        std::cout << "algorithm: " << opts.algorithm << std::endl;
        std::cout << "output_file: " << opts.output_file << std::endl;
    }
    // 確認是否成功解析 output_file
    /*if (!opts.output_file.empty()) {
        std::cout << "Output file: " << opts.output_file << std::endl;
    }
    else{
        std::cout << "Output file failure: " << opts.output_file << std::endl;
    }*/
}
