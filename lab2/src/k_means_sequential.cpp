#include "k_means_sequential.h"
#include "common.h"
#include <limits>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <iostream>
#include <vector>
#include "io.h"
#include <cmath>

// 計算歐幾里得距離的函數
real euclideanDistance(int dims, real *point, real *centroid) {
    real sum = 0.0;
    for (int i = 0; i < dims; i++) {
        sum += pow(point[i] - centroid[i], 2);  
    }
    return sqrt(sum); 
}

// Find the nearest centroid for each point
std::vector<int> findNearestCentroids(int n_points, int dims, real *data_points, real *centroids, int n_clusters) {
    std::vector<int> labels(n_points);
    
    for (int i = 0; i < n_points; i++) {
        int nearest_centroid = -1;
        real min_distance = std::numeric_limits<real>::max();

        for (int j = 0; j < n_clusters; j++) {
            real dist = euclideanDistance(dims, &data_points[i * dims], &centroids[j * dims]); 

            if (min_distance > dist) {
                min_distance = dist;
                nearest_centroid = j;
            }
        }
        labels[i] = nearest_centroid;
    }

    return labels;
}

// 累積每個聚類的點的總和
void updateCentroidSums(int n_points, int dims, real *data_points, const std::vector<int> &point_cluster_ids, int n_clusters, int *n_clusters_counts, real *new_centroids) {
    // 初始化新質心為零
    std::memset(new_centroids, 0, sizeof(real) * dims * n_clusters);
    // 初始化聚類計數為零
    std::memset(n_clusters_counts, 0, sizeof(int) * n_clusters);

    // 累積每個聚類的點的總和
    for (int i = 0; i < n_points; i++) {
        int cluster = point_cluster_ids[i];  // 獲取點所屬的聚類ID
        // 將該點的坐標加到對應質心的總和中
        for (int l = 0; l < dims; l++) {
            new_centroids[cluster * dims + l] += data_points[i * dims + l];
        }
        n_clusters_counts[cluster]++;  // 增加分配到此聚類的點的數量
    }
}

// 根據累積的總和計算新質心
void averageLabeledCentroids(int dims, int n_clusters, int *n_clusters_counts, real *new_centroids, real *data_points, int n_points) {
    // 對於每個質心，根據分配的點計算新質心
    for (int i = 0; i < n_clusters; i++) {
        if (n_clusters_counts[i] == 0) {
            // 若沒有點分配到此質心，則隨機選擇一個點作為新的質心
            int random_point = k_means_rand() % n_points;
            std::memcpy(&new_centroids[i * dims], &data_points[random_point * dims], sizeof(real) * dims);
        } else {
            // 計算分配到此質心的所有點的平均值
            for (int l = 0; l < dims; l++) {
                new_centroids[i * dims + l] /= n_clusters_counts[i];  // 將總和除以點的數量
            }
        }
    }
}


// Compute new centroids based on cluster assignments
void compute_new_centroids_bak(int n_points, int dims, real *data_points, const std::vector<int> &point_cluster_ids, int n_clusters, int *n_clusters_counts, real *new_centroids) {
    // Initialize new centroids to zero
    std::memset(new_centroids, 0, sizeof(real) * dims * n_clusters);
    // Initialize cluster counts to zero
    std::memset(n_clusters_counts, 0, sizeof(int) * n_clusters);

    // Accumulate the sum of points for each cluster
    for (int i = 0; i < n_points; i++) {
        int cluster = point_cluster_ids[i];  // Get the cluster ID for the point
        // Add the point's coordinates to the corresponding centroid's sum
        for (int l = 0; l < dims; l++) {
            new_centroids[cluster * dims + l] += data_points[i * dims + l];
        }
        n_clusters_counts[cluster]++;  // Increment the count of points assigned to this cluster
    }

    // For each centroid, find the point that is farthest from the current centroid
    for (int i = 0; i < n_clusters; i++) {
        if (n_clusters_counts[i] == 0) {
            // If no points are assigned to this centroid, randomly select a point to be the new centroid
            int random_point = k_means_rand() % n_points;
            std::memcpy(&new_centroids[i * dims], &data_points[random_point * dims], sizeof(real) * dims);
        } else {
            // 否則，計算每個群集中與 centroid 的最遠點
            real max_distance = -1.0;
            int farthest_point_idx = -1;

            for (int j = 0; j < n_points; j++) {
                if (point_cluster_ids[j] == i) {
                    real distance = euclideanDistance(dims, &data_points[j * dims], &new_centroids[i * dims]);
                    if (distance > max_distance) {
                        max_distance = distance;
                        farthest_point_idx = j;  // 找到最遠的點
                    }
                }
            }

            // 如果找到最遠點，將最遠點作為新的 centroid
            if (farthest_point_idx != -1) {
                std::memcpy(&new_centroids[i * dims], &data_points[farthest_point_idx * dims], sizeof(real) * dims);
            } else {
                // 如果沒有找到最遠點，則使用平均值
                for (int l = 0; l < dims; l++) {
                    new_centroids[i * dims + l] /= n_clusters_counts[i];  // Divide the sum by the number of points
                }
            }
        }
    }
}


// Check if centroids have converged
bool check_convergence(int n_clusters, int dims, real threshold, real *centroids_old, real *centroids_new) {
    bool converged = true;
    real l1=threshold/dims;
    for (int i = 0; i < n_clusters * dims; i++) {
        if (std::abs(centroids_old[i] - centroids_new[i]) >= l1) {
            converged = false;
            break;
        }
    }

    return converged;
}
// K-means 序列算法
int k_means_sequential(int n_points, real *data_points, struct options_t opts, int *point_cluster_ids, real *centroids, double& per_iteration_time) {
    std::srand(opts.seed);  // 使用用戶提供的種子
    real *centroids_old = centroids;
    real *centroids_new = new real[opts.n_clusters * opts.dimensions];
    int *cluster_point_counts = new int[opts.n_clusters];

    bool converged = false;
    int iterations = 0;

    // 開始計時
    auto start = std::chrono::high_resolution_clock::now();

    while (!converged && iterations < opts.max_iterations) {
        // 第一步：使用 findNearestCentroids 將 data_points 指派到最近的質心
        // Map each point to its nearest centroid
        std::vector<int> labels = findNearestCentroids(n_points, opts.dimensions, data_points, centroids_old, opts.n_clusters);

        // 將聚類分配從向量複製到陣列
        std::memcpy(point_cluster_ids, labels.data(), n_points * sizeof(int));

        // 第二步：累積每個聚類的點的總和
        updateCentroidSums(n_points, opts.dimensions, data_points, labels, opts.n_clusters, cluster_point_counts, centroids_new);

        // 第三步：計算新的質心
        averageLabeledCentroids(opts.dimensions, opts.n_clusters, cluster_point_counts, centroids_new, data_points, n_points);

        // 第三步：使用 opts.threshold 作為 epsilon 檢查收斂情況
        converged = check_convergence(opts.n_clusters, opts.dimensions, opts.threshold, centroids_old, centroids_new);

        // 將 centroids_new 複製回 centroids_old
        std::memcpy(centroids_old, centroids_new, sizeof(real) * opts.n_clusters * opts.dimensions);

        iterations++;
    }

    if (opts.debug) {
        // 調用函數檢查並打印結果
        check_and_print_results(opts.n_clusters, opts.dimensions, 
                                centroids_old, centroids_new, 
                                opts.threshold);
    }

    // 清理內存
    delete[] centroids_new;
    delete[] cluster_point_counts;

    // 結束計時
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> time_taken = end - start;

    per_iteration_time = time_taken.count() / iterations;

    print_ExecuteTime(iterations, per_iteration_time, time_taken.count());
    return iterations;
}
