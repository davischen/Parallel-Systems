#include "io.h"
#include "common.h"
#include <fstream>
#include <iostream>
#include <cassert>
#include <climits>
#include <cstdlib>

void read_file(struct options_t* opts, int* num_points, real **points_data) {
    unsigned int i = 0;
    int dim = 0;

    // 打開輸入檔案
    std::ifstream in;
    in.open(opts->in_file, std::ios::in);
    if (!in) {
        std::cerr << "Error: Could not open file " << opts->in_file << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // 讀取點數量
    in >> *num_points;
    if (*num_points <= 0 || *num_points > INT_MAX) {
        std::cerr << "Invalid number of points_data in input file " << std::endl;
        std::exit(1);
    }

    // 分配記憶體以存儲點數據
    *points_data = (real *)malloc((*num_points * opts->dimensions) * sizeof(real));
    if (!(*points_data)) {
        std::cerr << "Memory allocation failed" << std::endl;
        std::exit(1);
    }

    // 逐行讀取點數據
    for (i = 0; i < *num_points; i++) {
        int LineNo;
        in >> LineNo;  // 讀取行號
        //assert(LineNo == (i + 1));  // 檢查行號是否正確

        // 讀取每個點的各個維度的數據
        for (dim = 0; dim < opts->dimensions; dim++) {
            in >> (*points_data)[(i * opts->dimensions) + dim];
        }
    }

    in.close();  // 關閉檔案
}
