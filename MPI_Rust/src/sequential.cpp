#include "sequential.h"
#include <cmath>
#include <chrono>
#include <iostream>
#include "common.h"
#include "tree.h"
#include <cstring> // For memset
#include <vector>
#include <array>
#include <mpi.h> // For MPI_Wtime()
#include "io.h"
#include <cstdlib>
#include <iostream>
#include <thread> // for std::this_thread::sleep_for
//#include "visualization.h"

int BHSeq(const options_t* opts) {
    particle* p = nullptr; // 使用新的 particle 結構體
    Node* root;
    int n_p = 0;
    
    auto start = std::chrono::high_resolution_clock::now();

    // 讀取粒子數據
    //DEBUG_PRINT(std::cout << "Reading particle data from file: " << opts->in_file << std::endl);
    read_file(opts, &n_p, &p);
    //DEBUG_PRINT(std::cout << "Number of particles: " << n_p << std::endl);
    
    auto start_time = MPI_Wtime();//std::chrono::high_resolution_clock::now();

    double build_tree_timing = 0.0f,
           compute_force_timing = 0.0f,
           update_state_timing = 0.0f,
           free_tree_timing = 0.0f;

    double dt = opts->timestep; // 時間步長

    for (int s = 0; s < opts->n_steps; ++s) {
        auto iteration_start = std::chrono::high_resolution_clock::now();

        // 初始化根節點
        root = (Node*)malloc(sizeof(Node));
        initialize_root(root);
        // 插入粒子
        for (int i = 0; i < n_p; ++i) {
            particle* body = &p[i];
            insertBody(opts, root, body);
        }
        //printTree(root);
        build_tree_timing += std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now() - iteration_start).count() / NS_PER_MS;

        iteration_start = std::chrono::high_resolution_clock::now();

        // 計算粒子之間的力
        std::vector<std::array<double, 2>> forces(n_p, {0, 0});
        for (int i = 0; i < n_p; i++) {
            particle* body = &p[i];
            forces[i] = compute_force_v2(opts, root, body);
        }

        compute_force_timing += std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now() - iteration_start).count() / NS_PER_MS;

        iteration_start = std::chrono::high_resolution_clock::now();

        // 更新粒子位置與速度
        for (int i = 0; i < n_p; ++i) {
            particle* body = &p[i];
            updateParticleState_v2(body,forces[i], dt , root);
        }

        update_state_timing += std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now() - iteration_start).count() / NS_PER_MS;

        iteration_start = std::chrono::high_resolution_clock::now();

        // 可視化當前狀態
        /*if (opts->visualization) {
            visualization_render(n_p, p, root,s);
        }*/
        // 釋放樹的資源
        tearDownTree(root);

        free_tree_timing += std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now() - iteration_start).count() / NS_PER_MS;
    }

    auto stop_time = MPI_Wtime();//std::chrono::high_resolution_clock::now();
    //auto diff_loop = stop_time - start_time;//std::chrono::duration_cast<std::chrono::milliseconds>(stop_time - start_time);
    double execution_time_seconds = stop_time - start_time; // 轉換為秒
    printf("%f\n", execution_time_seconds);

    // 寫入結果到文件
    //DEBUG_PRINT(std::cout << "Writing results to output file: " << opts->out_file << std::endl);
    write_file(opts, n_p, p);

    // 釋放粒子數據
    if (p) free(p);

    // 計算總時間
    auto end = std::chrono::high_resolution_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    /*TIMING_PRINT(printf("Build tree: %lf ms\n", build_tree_timing / opts->n_steps));
    TIMING_PRINT(printf("Compute force: %lf ms\n", compute_force_timing / opts->n_steps));
    TIMING_PRINT(printf("Update state: %lf ms\n", update_state_timing / opts->n_steps));
    TIMING_PRINT(printf("Free tree: %lf ms\n", free_tree_timing / opts->n_steps));
    TIMING_PRINT(printf("Overall per iteration: %lf ms\n", diff.count() / opts->n_steps));
    TIMING_PRINT(printf("Overall time: %lf ms\n", diff.count()));*/
    /*if (opts->visualization) {
        terminate_visualization(); // 釋放 OpenGL 資源
    }*/

    MPI_Finalize();
    return 0;
}
