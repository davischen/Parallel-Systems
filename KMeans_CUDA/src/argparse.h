#pragma once
#include <string>

// 定義 options_t 結構
struct options_t {
    int n_clusters;
    int dimensions;
    std::string in_file;
    int max_iterations;
    double threshold;
    int seed;
    bool print_centroids;
    int algorithm;
    bool debug;  // 新增 debug 參數
    std::string output_file;  // 新增 output_file
};

// 解析命令列參數的函數
void parse_args(int argc, char **argv, options_t &opts);
