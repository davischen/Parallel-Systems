#include <cuda_runtime.h>
#include <iostream>
#include <cmath>
#include <cfloat>  
#include "common.h"
#include "argparse.h"  
#include "io.h"
#include <cuda_runtime.h>
#include <cuda.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>

// 定義一個宏來檢查 CUDA 調用是否成功
#define CHECK_ERROR_SHAREMEM(call) {                                                           \
    const cudaError_t error = call;                                             \
    if (error != cudaSuccess) {                                                 \
        printf("Error: %s:%d, ", __FILE__, __LINE__);                           \
        printf("code:%d, reason: %s\n", error, cudaGetErrorString(error));      \
        exit(1);                                                                \
    }                                                                           \
}

__device__ void loadCentersToShmem_bak(double* centers, double* temp_centers, int n_clusters, int dim, int threadsPerBlock, int local_index) {
    // 設定共享記憶體的大小
    int temp_centers_size = n_clusters * dim;
    
    // 計算每個執行緒需要加載的區段大小
    double window = ceil((double)temp_centers_size / (double)threadsPerBlock);
    
    // 將質心數據從全局記憶體加載到共享記憶體
    int i = 0;
    int index_start = local_index * (int)window;
    while (index_start < temp_centers_size && index_start + i < temp_centers_size) {
        temp_centers[index_start + i] = centers[index_start + i];
        if (++i == window) {
            i = 0;
            break;
        }
    }

    // 同步所有執行緒，確保共享記憶體加載完畢
    __syncthreads();
}

__device__ void loadCentersToShmem_bak2(double* centers, double* temp_centers, int n_clusters, int dim, int threadsPerBlock, int local_index) {
    // 計算每個執行緒需要加載的區段大小
    int temp_centers_size = n_clusters * dim;
    int window = (temp_centers_size + threadsPerBlock - 1) / threadsPerBlock; // rounding up

    // 將質心數據從全局記憶體加載到共享記憶體
    int index_start = local_index * window;
    for (int i = 0; i < window && index_start + i < temp_centers_size; ++i) {
        temp_centers[index_start + i] = centers[index_start + i];
    }

    // 同步所有執行緒，確保共享記憶體加載完畢
    __syncthreads();
}

__device__ void loadCentersToShmem(double* centers, double* temp_centers, int n_clusters, int dim, int threadsPerBlock, int local_index) {
    // 每個執行緒負責加載一個質心數據到共享記憶體
    if (local_index < n_clusters) {
        for (int i = 0; i < dim; i++) {
            temp_centers[local_index * dim + i] = centers[local_index * dim + i];
        }
    }

    // 確保所有執行緒都完成數據加載
    __syncthreads();
}

// 檢查是否所有聚類都已經收斂 Check if all clusters have converged
bool check_convergence_sharemem(double* convergence_k, double threshold, int n_clusters) {
    bool converge_result = true;
    for (int i = 0; i < n_clusters; i++) {
        if (convergence_k[i] > threshold) { 
            converge_result = false;
            return converge_result;
        }
    }
    return converge_result;
}

// 定義計算歐式距離的函數 Define a function that calculates Euclidean distance
__device__ double euclideanDistance_sharemem(int dims, double *point, double *centroid) {
    double sum = 0.0;
    for (int i = 0; i < dims; i++) {
        sum += (point[i] - centroid[i]) * (point[i] - centroid[i]); 
    }
    return sqrt(sum); 
}

// 找到每個點最近的聚類質心 Find the nearest cluster centroid for each point
__global__ void findNearestCentroids_sharemem(double* points, double* centroids, int* labels, int dims, int n_clusters, int num_points) {
    extern __shared__ double shared_centroids[];

    int point_label_index = threadIdx.x + blockIdx.x * blockDim.x;
    int thread_index = threadIdx.x;

    // 加載質心數據到 shared memory
    //for (int i = thread_index; i < n_clusters * dims; i += blockDim.x) {
    //    shared_centroids[i] = centroids[i];
    //}
    //__syncthreads(); // 確保所有線程都已經完成加載到 shared memory

    // Setup shared memory
    extern __shared__ double shared_centroids[];

    // 調用剛剛定義的函數來加載質心到共享記憶體
    loadCentersToShmem(centroids, shared_centroids, n_clusters, dims, blockDim.x, thread_index);

    if (point_label_index < num_points) {
        int point_index = point_label_index * dims;
        int nearest_centroid = -1;
        double min_distance = DBL_MAX;

        // 計算每個質心的歐式距離
        //centroid_0: [x0, y0, z0] -> indices 0, 1, 2
        //centroid_1: [x1, y1, z1] -> indices 3, 4, 5
        //centroid_2: [x2, y2, z2] -> indices 6, 7, 8
        //centroid_3: [x3, y3, z3] -> indices 9, 10, 11
        for (int j = 0; j < n_clusters; j++) {
            double distance = euclideanDistance_sharemem(dims, &points[point_index], &shared_centroids[j * dims]);

            if (distance < min_distance) {
                min_distance = distance;
                nearest_centroid = j;
            }
        }

        labels[point_label_index] = nearest_centroid;  // 記錄最接近的質心標籤
    }
}

// 將每個點的坐標加到對應質心的累加和中，並增加質心的點數 Add the coordinates of each point to the accumulated sum of the corresponding centroid
__global__ void updateCentroidSums_sharemem(double* points, double* centroids, int* labels, int* cluster_label_count, int dims, int num_points) {
    //extern __shared__ double shared_centroids[]; // shared memory
    //int local_index = threadIdx.x;
    
    int point_id = threadIdx.x + blockIdx.x * blockDim.x;

    if (point_id >= num_points) {
        return;
    }
    int cluster_id = labels[point_id];
    int points_start_index = point_id * dims;
    int center_start_index = cluster_id * dims;

    // 將每個點的坐標加到對應質心中
    for (int d = 0; d < dims; d++) {
        atomicAdd(&centroids[center_start_index + d], points[points_start_index + d]);
    }
    atomicAdd(&cluster_label_count[cluster_id], 1);  // 增加對應質心的點數
}

// 核計算每個質心的平均值（將累加值除以點的數量）Calculate the average of each centroid
__global__ void averageLabeledCentroids_sharemem(double* centroids, int* cluster_label_count, int dims, int n_clusters) {
    extern __shared__ double shared_centroids[]; // shared memory
    int local_index = threadIdx.x;

    // 將質心數據加載到共享記憶體
    loadCentersToShmem(centroids, shared_centroids, n_clusters, dims, blockDim.x, local_index);

    int cluster_id = threadIdx.x + blockIdx.x * blockDim.x;
    if (cluster_id >= n_clusters) {
        return;
    }
    int cluster_start_index = cluster_id * dims;
    for (int d = 0; d < dims; d++) {
        centroids[cluster_start_index + d] = shared_centroids[cluster_start_index + d] / cluster_label_count[cluster_id];
    }
}

__global__ void convergence_sharemem(double* centroids, double* old_centroids, double* convergence_k, int dims, int n_clusters) {
    int thread_index = threadIdx.x;
    extern __shared__ double shared_centroids[]; // 使用 shared memory 儲存質心數據

    // 調用剛剛定義的函數來加載質心到共享記憶體
    loadCentersToShmem(old_centroids, shared_centroids, n_clusters, dims, blockDim.x, thread_index);

    //extern __shared__ double shared_centroids[]; // shared memory

    int cluster_id = threadIdx.x + blockIdx.x * blockDim.x;
    if (cluster_id >= n_clusters) {
        return;
    }

    double cluster_converge_sum = 0;
    int ci_start = cluster_id * dims;
    for (int d = 0; d < dims; d++) {
        cluster_converge_sum += fabs((centroids[ci_start + d] - shared_centroids[ci_start + d]) / shared_centroids[ci_start + d] * 100);
    }
    convergence_k[cluster_id] = cluster_converge_sum;
}

int k_means_cuda_shared(int n_points, real* data_points, struct options_t opts, int* point_cluster_ids, real* centroids, double& per_iteration_time) {
    // 計算每個數組所需的記憶體大小
    size_t points_size = n_points * opts.dimensions * sizeof(real); 
    size_t centroids_size = opts.n_clusters * opts.dimensions * sizeof(real);  
    size_t labels_size = n_points * sizeof(int);  
    size_t cluster_count_size = opts.n_clusters * sizeof(int);  
    size_t convergence_size = opts.n_clusters * sizeof(double); 

    // 定義GPU上的變量，這些變量將被分配記憶體
    real *d_points, *d_centroids, *d_old_centroids, *d_convergence_k;
    int *d_point_cluster_label, *d_cluster_label_count;
    double* convergence_k = new double[opts.n_clusters] { 0 };

    // 計時器初始化
    cudaEvent_t start_transfer, stop_transfer, start_total, stop_total, start_iter, stop_iter;
    float transfer_time = 0.0, total_runtime = 0.0;

    // 初始化收斂標誌和迭代計數器
    bool converged = false;
    int iterations = 0;
    float time_step;

    // 建立 CUDA 事件以進行計時 Create CUDA events for timing
    cudaEventCreate(&start_transfer);
    cudaEventCreate(&stop_transfer);
    cudaEventCreate(&start_total);
    cudaEventCreate(&stop_total);
    cudaEventCreate(&start_iter);
    cudaEventCreate(&stop_iter);
    
    // 開始計時總運轉時間 Start timing total runtime
    cudaEventRecord(start_total, 0);

    // 在GPU上分配記憶體
    CHECK_ERROR_SHAREMEM(cudaMalloc((void**) &d_points, points_size)); 
    CHECK_ERROR_SHAREMEM(cudaMalloc((void**) &d_centroids, centroids_size));  
    CHECK_ERROR_SHAREMEM(cudaMalloc((void**) &d_point_cluster_label, labels_size)); 
    CHECK_ERROR_SHAREMEM(cudaMalloc((void**) &d_cluster_label_count, cluster_count_size)); 
    CHECK_ERROR_SHAREMEM(cudaMalloc((void**) &d_convergence_k, convergence_size)); 
    CHECK_ERROR_SHAREMEM(cudaMalloc((void**) &d_old_centroids, centroids_size)); 

    // 將資料從主機（CPU）傳輸到設備（GPU）
    //cudaError_t cudaMemcpy(void *dst, const void *src, size_t count, cudaMemcpyKind kind);
    cudaEventRecord(start_transfer, 0);
    CHECK_ERROR_SHAREMEM(cudaMemcpy(d_points, data_points, points_size, cudaMemcpyHostToDevice));  
    CHECK_ERROR_SHAREMEM(cudaMemcpy(d_centroids, centroids, centroids_size, cudaMemcpyHostToDevice)); 
    cudaEventRecord(stop_transfer, 0);
    cudaEventSynchronize(stop_transfer);

    // 計算從主機到設備的傳輸時間 Calculate transfer time from host to device
    float host_to_device_transfer_time;
    cudaEventElapsedTime(&host_to_device_transfer_time, start_transfer, stop_transfer);
    transfer_time += host_to_device_transfer_time;

    // 設置每個block的執行線程數以及block的總數
    int threadsPerBlock = 256;  // 每個block的線程數
    int n_blocksPerGrid = (n_points + threadsPerBlock - 1) / threadsPerBlock;  // 每個點對應的block數量 (N+block size -1)/block size
    int n_blocks_cluster = (opts.n_clusters + threadsPerBlock - 1) / threadsPerBlock;  // 每個聚類對應的block數量

    if(opts.debug or PERFORMANCE_TEST) {
        printf("Step 0 threadsPerBlock_PerBlock: %d\n", threadsPerBlock);          
        printf("Step 0 blocks points Per Grid: %d\n", n_blocksPerGrid);           
        printf("Step 0 cluster by block: %d\n", n_blocks_cluster); 
        printf("Step 0 data points: %d\n", n_points);       
    }

    // 開始K-means迭代，直到達到最大迭代次數或收斂
    while (!converged && iterations < opts.max_iterations) {
        // 開始計時
        cudaEventRecord(start_iter, 0);
        iterations++;  // 每次進行迭代時增加計數

        // 1. 更新每個資料點的最近質心標籤 Calculating Euclidean Distance and Assigning data points to closest centroids
        // Map each point to its nearest centroid
        findNearestCentroids_sharemem<<<n_blocksPerGrid, threadsPerBlock, opts.n_clusters * opts.dimensions * sizeof(double)>>>(d_points, d_centroids, d_point_cluster_label, opts.dimensions, opts.n_clusters, n_points);
        CHECK_ERROR_SHAREMEM(cudaDeviceSynchronize()); 
        
        if(PERFORMANCE_TEST) {
            cudaEventRecord(stop_iter);  
            cudaEventSynchronize(stop_iter);
            cudaEventElapsedTime(&time_step, start_iter, stop_iter);
            printf("Step 1 (findNearestCentroids): %f ms\n", time_step);
            cudaEventRecord(start_iter);
        }

        // 2. 保存舊的質心以便於收斂檢查 Copy centroids_new back to centroids_old
        CHECK_ERROR_SHAREMEM(cudaMemcpy(d_old_centroids, d_centroids, centroids_size, cudaMemcpyDeviceToDevice));

        // 3. 重置新的質心和每個聚類的點數統計
        //cudaError_t cudaMemset(void *devPtr, int value, size_t count);
        cudaMemset(d_centroids, 0, centroids_size);  // 清空質心
        cudaMemset(d_cluster_label_count, 0, cluster_count_size);  // 清空聚類點數

        // 4. 累加點到各個質心的總和 
        updateCentroidSums_sharemem<<<n_blocksPerGrid, threadsPerBlock, opts.n_clusters * opts.dimensions * sizeof(double)>>>(d_points, d_centroids, d_point_cluster_label, d_cluster_label_count, opts.dimensions, n_points);
        CHECK_ERROR_SHAREMEM(cudaDeviceSynchronize());
        
        if(PERFORMANCE_TEST) {
            cudaEventRecord(stop_iter); 
            cudaEventSynchronize(stop_iter);
            cudaEventElapsedTime(&time_step, start_iter, stop_iter);
            printf("Step 2 (addLabeledCentroids_Cuda): %f ms\n", time_step);
            cudaEventRecord(start_iter);
        }

        // 5. 計算每個聚類的平均質心
        averageLabeledCentroids_sharemem<<<n_blocks_cluster, threadsPerBlock, opts.n_clusters * opts.dimensions * sizeof(double)>>>(d_centroids, d_cluster_label_count, opts.dimensions, opts.n_clusters);
        CHECK_ERROR_SHAREMEM(cudaDeviceSynchronize());

        // 6. 計算質心的收斂程度
        cudaMemset(d_convergence_k, 0, convergence_size);  // 初始化收斂數據
        convergence_sharemem<<<n_blocks_cluster, threadsPerBlock, opts.n_clusters * opts.dimensions * sizeof(double)>>>(d_centroids, d_old_centroids, d_convergence_k, opts.dimensions, opts.n_clusters);
        CHECK_ERROR_SHAREMEM(cudaDeviceSynchronize());

        // 將收斂數據從GPU拷貝回主機
        CHECK_ERROR_SHAREMEM(cudaMemcpy(convergence_k, d_convergence_k, convergence_size, cudaMemcpyDeviceToHost));
    
        // 7. 檢查是否達到收斂條件
        converged = check_convergence_sharemem(convergence_k, opts.threshold, opts.n_clusters);

        // 結束本次迭代的計時
        cudaEventRecord(stop_iter, 0);
        cudaEventSynchronize(stop_iter);

        if(PERFORMANCE_TEST) {
            // 打印聚類的點數統計
            //int* h_cluster_label_count = (int*)malloc(cluster_count_size);
            //CHECK_ERROR_SHAREMEM(cudaMemcpy(h_cluster_label_count, d_cluster_label_count, cluster_count_size, cudaMemcpyDeviceToHost));
            //print_ClusterLabelCount(h_cluster_label_count, opts.n_clusters);
            //free(h_cluster_label_count); 
            // 將質心從 GPU 拷貝回主機並打印
            //real* h_centroids = (real*)malloc(centroids_size);
            //CHECK_ERROR_SHAREMEM(cudaMemcpy(h_centroids, d_centroids, centroids_size, cudaMemcpyDeviceToHost));
            //print_Centers(h_centroids, opts.n_clusters, opts.dimensions);
            //free(h_centroids); 
            cudaEventElapsedTime(&time_step, start_iter, stop_iter);
            printf("Step 3 (convergence_Cuda): %f ms\n", time_step);
        }

        // 計算本次迭代的時間並累加
        //float iteration_time;
        //cudaEventElapsedTime(&iteration_time, start_iter, stop_iter);
        //time_taken += iteration_time;

    }

    // 將結果從設備拷貝回主機
    cudaEventRecord(start_transfer, 0);
    CHECK_ERROR_SHAREMEM(cudaMemcpy(point_cluster_ids, d_point_cluster_label, labels_size, cudaMemcpyDeviceToHost));  // 標籤結果
    CHECK_ERROR_SHAREMEM(cudaMemcpy(centroids, d_centroids, centroids_size, cudaMemcpyDeviceToHost));  // 最終質心結果
    cudaEventRecord(stop_transfer, 0);
    cudaEventSynchronize(stop_transfer);

    // 計算從設備到主機的傳輸時間 Calculate transfer time from device to host
    float device_to_host_transfer_time;
    cudaEventElapsedTime(&device_to_host_transfer_time, start_transfer, stop_transfer);
    transfer_time += device_to_host_transfer_time;

    // Record the end of the total runtime
    cudaEventRecord(stop_total, 0);
    cudaEventSynchronize(stop_total);

    // 計算總運行時間 Calculate total runtime
    cudaEventElapsedTime(&total_runtime, start_total, stop_total);
    // 計算總運行時間 Calculate total runtime
    cudaEventElapsedTime(&total_runtime, start_total, stop_total);
    per_iteration_time = total_runtime / iterations;
    float time_taken = total_runtime - transfer_time;

    // 釋放GPU上的記憶體資源
    cudaFree(d_points);
    cudaFree(d_centroids);
    cudaFree(d_old_centroids);
    cudaFree(d_convergence_k);
    cudaFree(d_point_cluster_label);
    cudaFree(d_cluster_label_count);

    // 釋放計時器
    cudaEventDestroy(start_transfer);
    cudaEventDestroy(stop_transfer);
    cudaEventDestroy(start_total);
    cudaEventDestroy(stop_total);
    cudaEventDestroy(start_iter);
    cudaEventDestroy(stop_iter);

    // Print debug information
    if(opts.debug or PERFORMANCE_TEST) {
        std::cout << "Total runtime: " << total_runtime << " ms" << std::endl;
        std::cout << "Total data transfer time: " << transfer_time << " ms" << std::endl;
        std::cout << "Fraction of time spent on data transfer: " << (transfer_time / total_runtime) << std::endl;
        std::cout << "Algorithm Time taken by function: " << time_taken << " ms" << std::endl;
    }

    // 將最終的迭代時間平均值返回給調用者
    per_iteration_time = time_taken / iterations;

    print_ExecuteTime(iterations, per_iteration_time,time_taken);
    // 返回總的迭代次數
    return iterations;
}

