#include "io.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <fstream>
#include <climits>  // 定義 INT_MAX

void read_file_parallel(struct options_t* opts, struct particle **bodies, int num_procs) {
    // 打開輸入文件
    std::ifstream in;
    in.open(opts->in_file);

    // 獲取粒子數量
    in >> opts->n_particles;

    if ((opts->n_particles) <= 0 || (opts->n_particles) > INT_MAX) {
        std::cerr << "Invalid number of inputs in input file" << std::endl;
        exit(1);
    }

    // 處理粒子分組
    int subGroupRem, totalBodies;
    totalBodies = opts->n_particles;
    if (num_procs > 1) {
        subGroupRem = opts->n_particles % num_procs;
        totalBodies = opts->n_particles + ((num_procs - subGroupRem) % num_procs);
    }
    opts->n_bodiesParallel = totalBodies;

    // 分配內存
    int bodySize = opts->n_bodiesParallel * sizeof(struct particle);
    *bodies = (struct particle *)malloc(bodySize);
    memset((char *)(*bodies), 0, bodySize);

    // 設置邊界條件
    double min[2], max[2];
    min[0] = min[1] = 0;
    max[0] = max[1] = 4;

    // 讀取粒子數據
    for (int i = 0; i < opts->n_particles; ++i) {
        struct particle *b = &((*bodies)[i]);
        in >> b->index;
        in >> b->x;
        in >> b->y;
        in >> b->mass;
        in >> b->v_x;
        in >> b->v_y;
        if (b->x < min[0] || b->y < min[1])
            b->mass = -1;
        if (b->x > max[0] || b->y > max[1])
            b->mass = -1;
    }

    // 填充剩餘粒子數據（用於並行對齊）
    for (int i = opts->n_particles; i < opts->n_bodiesParallel; i++) {
        struct particle *b = &((*bodies)[i]);
        b->index = -10;
    }
}

void write_file_parallel(struct options_t* opts, struct particle *bodies) {
    // 打開輸出文件
    std::ofstream out;
    out.open(opts->out_file, std::ofstream::trunc);

    // 輸出粒子數量
    out << opts->n_particles << std::endl;

    // 設定精度
    out << std::fixed;
    out.precision(17);

    // 寫入粒子數據
    for (int i = 0; i < opts->n_particles; ++i) {
        struct particle *b = &bodies[i];
        out << b->index << " ";
        out << b->x << " ";
        out << b->y << " ";
        out << b->mass << " ";
        out << b->v_x << " ";
        out << b->v_y;
        out << std::endl;
    }

    out.flush();
    out.close();
}

void read_file(const options_t* args, int* n_particles, particle** particles) {
    // 打開輸入文件
    FILE* input_f = fopen(args->in_file.c_str(), "r");
    if (!input_f) {
        std::cerr << "Error: Unable to open input file " << args->in_file << std::endl;
        exit(EXIT_FAILURE);
    }

    // 讀取粒子數量
    int scanned = fscanf(input_f, "%d", n_particles);
    if (scanned != 1) {
        std::cerr << "Error reading number of particles! Scanned: " << scanned << std::endl;
        fclose(input_f);
        exit(EXIT_FAILURE);
    }

    *particles = (particle*)malloc(*n_particles * sizeof(particle));

    // 讀取每個粒子數據
    for (int i = 0; i < *n_particles; ++i) {
        scanned = fscanf(input_f, "%d\t%lf\t%lf\t%lf\t%lf\t%lf",
                         &((*particles)[i].index),
                         &((*particles)[i].x),
                         &((*particles)[i].y),
                         &((*particles)[i].mass),
                         &((*particles)[i].v_x),
                         &((*particles)[i].v_y));
        if (scanned != 6) {
            std::cerr << "Error reading particle! Scanned: " << scanned << std::endl;
            fclose(input_f);
            free(*particles);
            exit(EXIT_FAILURE);
        }
    }

    fclose(input_f);
}

void write_file(const options_t* args, int n_particles, const particle* particles) {
    // 打開輸出文件
    FILE* output_f = fopen(args->out_file.c_str(), "w");
    if (!output_f) {
        std::cerr << "Error: Unable to open output file " << args->out_file << std::endl;
        exit(EXIT_FAILURE);
    }

    // 輸出粒子數量
    fprintf(output_f, "%d\n", n_particles);

    // 寫入粒子數據
    for (int i = 0; i < n_particles; ++i) {
        fprintf(output_f, "%d\t%.17lf\t%.17lf\t%.17lf\t%.17lf\t%.17lf\n",
                particles[i].index,
                particles[i].x,
                particles[i].y,
                particles[i].mass,
                particles[i].v_x,
                particles[i].v_y);
    }

    fclose(output_f);
}
