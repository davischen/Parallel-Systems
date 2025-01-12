#include <iostream>
#include <fstream>  // 用於文件輸出
#include <chrono>
#include "argparse.h"
#include "io.h"
#include "common.h"
#include "k_means_sequential.h"
#include "k_means_cuda.h"
#include "k_means_cuda_shared.h"
#include "k_means_thrust.h"

int main(int argc, char **argv) {
    // 定義參數
    options_t opts;

    // 解析命令列參數
    parse_args(argc, argv, opts);

    // 讀取檔案
    int n_points;
    real *data_points;
    read_file(&opts, &n_points, &data_points);

    // 初始化記憶體
    int *point_cluster_ids = new int[n_points];
    real *centroids = new real[opts.n_clusters * opts.dimensions];
    //init_random_centroids(opts.n_clusters, opts.dimensions, n_points, data_points, centroids, opts.seed);
    init_kmeans_centroids(n_points, data_points, centroids, opts);

    int iterations = 0;
    double per_iteration_time = 0.0;

    // 計時開始
    auto start = std::chrono::high_resolution_clock::now();
    /*if(opts.debug)
    {
        for (int i = 0; i < opts.n_clusters; i++) {
            for (int j = 0; j < opts.dimensions; j++) {
                std::cout << centroids[i * opts.dimensions + j] << " ";
            }
            std::cout << std::endl;
        }
    }*/

    // 執行 K-Means 演算法
    //iterations = k_means_sequential(n_points, data_points, opts, point_cluster_ids, centroids);

    // 根據不同演算法執行對應版本的K-means
    switch (opts.algorithm)
    {
        case 0:
            if(opts.debug)
            {
                std::cout << "Running k_means_sequential..." << std::endl;
            }
            iterations = k_means_sequential(n_points, data_points, opts, point_cluster_ids, centroids, per_iteration_time);
            break;
        case 1:
            if(opts.debug)
            {
                std::cout << "Running k_means_thrust..." << std::endl;
            }
            //iterations = k_means_thrust(n_points, data_points, opts, point_cluster_ids, centroids, per_iteration_time);
            iterations = k_means_thrust_optimized(n_points,  data_points, opts, point_cluster_ids, centroids, per_iteration_time);
            break;
        case 2:
            if(opts.debug)
            {
                std::cout << "Running k_means_cuda..." << std::endl;
            }
            iterations = k_means_cuda(n_points, data_points, opts, point_cluster_ids, centroids, per_iteration_time);
            break;
        case 3:
            if(opts.debug)
            {
                std::cout << "Running k_means_cuda_shared..." << std::endl;
            }
            iterations = k_means_cuda_shared(n_points, data_points, opts, point_cluster_ids, centroids, per_iteration_time);
            break;
        /*case 4:
            if(opts.debug)
            {
                std::cout << "Running k_means_thrust_optimized..." << std::endl;
            }
            //iterations = k_means_thrust(n_points, data_points, opts, point_cluster_ids, centroids, per_iteration_time);
            iterations = k_means_thrust_test(n_points,  data_points, opts, point_cluster_ids, centroids, per_iteration_time);
            break;*/
        default:
            //std::cerr << "Unknown algorithm!" << std::endl;
            return 1;
    }

    // 計時結束
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> diff = end - start;
    per_iteration_time = diff.count() / iterations;
    if(opts.debug)
    {
        //std::cout << "Iterations: " << iterations << std::endl;
        //std::cout << "Per iteration time: " << per_iteration_time << " ms" << std::endl;
        // 如果指定了 output_file，將結果寫入文件
        /*if (!opts.output_file.empty()) {
            std::ofstream output_file(opts.output_file);
            if (output_file.is_open()) {
                //output_file << "Iterations: " << iterations << "\n";
                //output_file << "Centroids:\n";
                for (int i = 0; i < opts.n_clusters; i++) {
                    output_file << i;
                    for (int j = 0; j < opts.dimensions; j++) {
                        output_file << " " << centroids[i * opts.dimensions + j];
                    }
                    output_file << "\n";
                }
                output_file.close();
            } else {
                std::cerr << "Unable to open output file: " << opts.output_file << std::endl;
            }
        }*/
    }
    //print_ExecuteTime(iterations, per_iteration_time);

    // 輸出結果到控制台
    if (opts.print_centroids) {
        // 輸出最終簇的質心
        print_Centroids(opts.n_clusters, opts.dimensions, centroids);
    } else {
        // 輸出每個點的最終簇ID
        print_ClusterAssignments(n_points, point_cluster_ids);
    }

    // 釋放記憶體
    delete[] centroids;
    delete[] point_cluster_ids;
    delete[] data_points;

    return 0;
}
