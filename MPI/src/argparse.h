#pragma once

#include <getopt.h>
#include <string>

// 定義參數結構
struct options_t {
    std::string in_file;    // 輸入文件
    std::string out_file;   // 輸出文件
    int n_steps;            // 模擬步數
    int n_bodiesParallel;
    int n_particles;               // 粒子數量
    double threshold;       // MAC 閾值
    double timestep;        // 時間步長
    bool visualization;     // 可視化標誌
    bool sequential;        // 是否使用序列模式
    std::string mpi_type;   
};

// 解析命令行參數
void get_opts(int argc, char** argv, options_t* opts);
