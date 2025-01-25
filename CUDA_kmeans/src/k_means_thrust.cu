#include <iostream>
#include <thrust/device_vector.h>
#include <thrust/iterator/discard_iterator.h>
#include <thrust/iterator/counting_iterator.h>
#include <thrust/iterator/transform_iterator.h>
#include <thrust/iterator/zip_iterator.h>
#include <thrust/reduce.h>
#include <thrust/copy.h>
#include <thrust/logical.h>
#include "k_means_thrust.h"
#include "common.h"

typedef thrust::device_vector<real> dv_real;
typedef thrust::device_vector<int> dv_int;
typedef thrust::host_vector<real> hv_real;

typedef thrust::tuple<real, int> real_indexed; // 新增 real_indexed 定義

struct maximum_by_first : public thrust::binary_function<real_indexed, real_indexed, real_indexed> {
    maximum_by_first() {}

    __host__ __device__
    real_indexed operator()(real_indexed x_1, real_indexed x_2) {
        return thrust::get<0>(x_1) < thrust::get<0>(x_2) ? x_1 : x_2;
    }
};

#define D_PRINT_POINT(x, d, i) { \
  if(DEBUG_TEST) {\
    thrust::copy_n( \
      x.begin() + i*d, \
      d, \
      std::ostream_iterator<real>(std::cerr, ", ") \
    ); \
    std::cerr << std::endl;\
  }\
}
#define D_PRINT_ALL(x) { \
  if(DEBUG_TEST) {\
    thrust::copy( \
      x.begin(), \
      x.end(), \
      std::ostream_iterator<real>(std::cerr, ", ") \
    ); \
    std::cerr << std::endl;\
  }\
}

// 定義計算歐式距離的函數 Define a function that calculates Euclidean distance
struct euclideanDistance : public thrust::unary_function<int, real> {
    const int d, k;
    const real* points, * centroids;

    euclideanDistance(int _d, int _k, real* _points, real* _centroids)
        : d(_d), k(_k), points(_points), centroids(_centroids) {}

    __host__ __device__
    real operator()(int index) {
        int i = (index / d) / k;
        int j = (index / d) % k;
        int l = index % d;
        return (points[i * d + l] - centroids[j * d + l]) * (points[i * d + l] - centroids[j * d + l]);
    }
};

// 將每個點的坐標加到對應質心的累加和中，並增加質心的點數 Add the coordinates of each point to the accumulated sum of the corresponding centroid
struct updateCentroidSums_thrust_op : public thrust::unary_function<void, thrust::tuple<real, int>> {
    const int d;
    const int* d_point_cluster_ids;
    real* centroids;

    updateCentroidSums_thrust_op(int _d, int* _d_point_cluster_ids, real* _centroids)
        : d(_d), d_point_cluster_ids(_d_point_cluster_ids), centroids(_centroids) {}

    __device__
    void operator()(thrust::tuple<real, int> real_index) {
        real value = thrust::get<0>(real_index);
        int index = thrust::get<1>(real_index);
        int target_centroid_id = d_point_cluster_ids[index / d];
        int target_centroid_component_id = target_centroid_id * d + index % d;
        atomicAdd(centroids + target_centroid_component_id, value);
    }
};

// 核計算每個質心的平均值（將累加值除以點的數量）Calculate the average of each centroid
struct averageLabeledCentroids_thrust_op : public thrust::unary_function<void, thrust::tuple<int, int>> {
    const int d, i;
    real* centroids;

    averageLabeledCentroids_thrust_op(int _d, int _i, real* _centroids)
        : d(_d), i(_i), centroids(_centroids) {}

    __device__
    void operator()(thrust::tuple<int, int> ints) {
        int centroid_id = thrust::get<0>(ints);
        int counts = thrust::get<1>(ints);
        *(centroids + centroid_id * d + i) /= counts;
    }
};

struct check_convergence_thrust_op : public thrust::unary_function<bool, thrust::tuple<real, real>> {
    const real threshold;
    check_convergence_thrust_op(real _threshold) : threshold(_threshold) {}

    __host__ __device__
    bool operator()(thrust::tuple<real, real> realz) {
        real x1 = thrust::get<0>(realz);
        real x2 = thrust::get<1>(realz);
        return abs(x1 - x2) < threshold;
    }
};

int k_means_thrust_optimized(int n_points, real* data_points, struct options_t opts, int* point_cluster_ids, real* centroids, double& per_iteration_time) {
    using namespace thrust;
    using namespace thrust::placeholders;

    bool done = false;
    int iterations = 0;
    //int k = opts.n_clusters;
    //int d = opts.dimensions;
    float time_step;

    // CUDA events for timing
    cudaEvent_t start_total, stop_total, start_transfer, stop_transfer;
    cudaEvent_t start_iter, stop_iter;  // Events for step timing
    float transfer_time = 0.0, total_runtime = 0.0;
    cudaEventCreate(&start_total);
    cudaEventCreate(&stop_total);
    cudaEventCreate(&start_transfer);
    cudaEventCreate(&stop_transfer);
    cudaEventCreate(&start_iter);
    cudaEventCreate(&stop_iter);

    cudaEventRecord(start_total, 0);  // Start overall timing

    // 將資料從主機（CPU）傳輸到設備（GPU） (Step 1)
    cudaEventRecord(start_transfer, 0);
    dv_real d_points(data_points, data_points + n_points * opts.dimensions);
    dv_real old_centroids(centroids, centroids + opts.n_clusters * opts.dimensions);

    cudaEventRecord(stop_transfer, 0);
    cudaEventSynchronize(stop_transfer);

    // 計算從主機到設備的傳輸時間 Calculate transfer time from host to device
    float host_to_device_transfer_time;
    cudaEventElapsedTime(&host_to_device_transfer_time, start_transfer, stop_transfer);
    transfer_time += host_to_device_transfer_time;

    // Create variables for computation
    dv_real new_centroids(opts.n_clusters * opts.dimensions);
    dv_real point_centroid_distances(n_points * opts.n_clusters);
    dv_int d_point_cluster_ids(n_points);
    dv_int point_to_centroid_map(n_points);
    dv_int d_k_counts(opts.n_clusters);
    dv_int d_k_count_keys(opts.n_clusters);

    // Step 2: K-means iteration loop
    // 開始K-means迭代，直到達到最大迭代次數或收斂
    while (!done) {
        // 開始計時每次迭代
        cudaEventRecord(start_iter, 0);  // Start timing for this iteration step

        //DEBUG_OUT("Old centroids:");
        //D_PRINT_ALL(old_centroids);

        // Compute distance components (Step 2.1)
        // 1. 更新每個資料點的最近質心標籤
        // Map each point to its nearest centroid
        euclideanDistance dist_comp(opts.dimensions, opts.n_clusters, raw_pointer_cast(d_points.data()), raw_pointer_cast(old_centroids.data()));

        reduce_by_key(
            make_transform_iterator(counting_iterator<int>(0), _1 / opts.dimensions), // Key input start: Transforms the index to represent a cluster based on the dimensional index
            make_transform_iterator(counting_iterator<int>(n_points * opts.n_clusters * opts.dimensions), _1 / opts.dimensions), // Key input end: Same transformation for the range end
            make_transform_iterator(counting_iterator<int>(0), dist_comp), // Value input: Computes some comparison metric using 'dist_comp' for each transformed index
            make_discard_iterator(), // Output key: Discards the output keys (not used)
            point_centroid_distances.begin() // Output value: Stores the results of the reduction (comparison values) in 'point_centroid_distances'
        );

        if (PERFORMANCE_TEST) {
            cudaEventRecord(stop_iter, 0);  // End timing for this step
            cudaEventSynchronize(stop_iter);
            cudaEventElapsedTime(&time_step, start_iter, stop_iter);
            printf("Step 2.1 (Distance computation): %f ms\n", time_step);
            cudaEventRecord(start_iter);  // Start timing for the next step
        }

        // Assign points to centroids (Step 2.2)
        // 2. 保存舊的質心以便於收斂檢查
        reduce_by_key(
            make_transform_iterator(counting_iterator<int>(0), _1 / opts.n_clusters), // Key input start: Maps indices to clusters by dividing by the number of clusters
            make_transform_iterator(counting_iterator<int>(n_points * opts.n_clusters), _1 / opts.n_clusters), // Key input end: Same mapping for the end of the range
            make_zip_iterator(make_tuple(point_centroid_distances.begin(), counting_iterator<int>(0))), // Value input: Combines the distance values with the point index
            make_discard_iterator(), // Output key: Discards the output keys (not used)
            make_zip_iterator(make_tuple(make_discard_iterator(), point_to_centroid_map.begin())), // Output value: Updates the 'point_to_centroid_map' with the closest centroid index
            equal_to<int>(), // Binary predicate: Uses equality to group keys for reduction
            maximum_by_first() // Binary operation: Chooses the maximum based on the first element of the tuple (distance comparison)
        );

        transform(
            point_to_centroid_map.begin(), // Input start: Beginning of the mapping of points to centroids
            point_to_centroid_map.end(),   // Input end: End of the mapping of points to centroids
            point_to_centroid_map.begin(), // Output: Updates the 'point_to_centroid_map' in place
            _1 % opts.n_clusters           // Unary operation: Takes the modulo of the centroid index with the number of clusters to ensure it remains within valid range
        );

        if (PERFORMANCE_TEST) {
            cudaEventRecord(stop_iter, 0);  
            cudaEventSynchronize(stop_iter);
            cudaEventElapsedTime(&time_step, start_iter, stop_iter);
            printf("Step 2.2 (Assign points to centroids): %f ms\n", time_step);
            cudaEventRecord(start_iter); 
        }

        // Zero the centroids and compute new centroids (Step 2.3)
        // 3. 重置新的質心和每個聚類的點數統計
        fill(new_centroids.begin(), new_centroids.end(), 0);

        updateCentroidSums_thrust_op compute_assign_means(opts.dimensions, raw_pointer_cast(point_to_centroid_map.data()), raw_pointer_cast(new_centroids.data()));
        for_each(
            make_zip_iterator(make_tuple(d_points.begin(), counting_iterator<int>(0))),
            make_zip_iterator(make_tuple(d_points.end(), counting_iterator<int>(n_points * opts.dimensions))),
            compute_assign_means
        );

        if (PERFORMANCE_TEST) {
            cudaEventRecord(stop_iter, 0);  
            cudaEventSynchronize(stop_iter);
            cudaEventElapsedTime(&time_step, start_iter, stop_iter);
            printf("Step 2.3 (Compute new centroids): %f ms\n", time_step);
            cudaEventRecord(start_iter);  
        }

        d_point_cluster_ids = point_to_centroid_map;

        // Sort point cluster IDs (Step 2.4)
        sort(d_point_cluster_ids.begin(), d_point_cluster_ids.end());

        auto new_end = reduce_by_key(
            d_point_cluster_ids.begin(), // Key input start: Iterator to the beginning of the cluster IDs for each point
            d_point_cluster_ids.end(),   // Key input end: Iterator to the end of the cluster IDs for each point
            make_constant_iterator(1),   // Value input: A constant iterator that generates the value 1 for each key
            d_k_count_keys.begin(),      // Output key: Start of the output range where the unique cluster IDs will be stored
            d_k_counts.begin()           // Output value: Start of the output range where the counts of points per cluster will be stored
        );

        fill(new_end.second, d_k_counts.end(), 0);

        if (PERFORMANCE_TEST) {
            cudaEventRecord(stop_iter, 0);  
            cudaEventSynchronize(stop_iter);
            cudaEventElapsedTime(&time_step, start_iter, stop_iter);
            printf("Step 2.4 (Sort cluster IDs and reduce): %f ms\n", time_step);
            cudaEventRecord(start_iter);  
        }

        // Divide centroids by point count (Step 2.5)
        // 5. 計算每個聚類的平均質心
        for (int i = 0; i < opts.dimensions; i++) {
            for_each(
                make_zip_iterator(make_tuple(d_k_count_keys.begin(), d_k_counts.begin())),
                make_zip_iterator(make_tuple(new_end.first, new_end.second)),
                averageLabeledCentroids_thrust_op(opts.dimensions, i, raw_pointer_cast(new_centroids.data()))
            );
        }

        if (PERFORMANCE_TEST) {
            cudaEventRecord(stop_iter, 0);  
            cudaEventSynchronize(stop_iter);
            cudaEventElapsedTime(&time_step, start_iter, stop_iter);
            printf("Step 2.5 (Divide centroids by counts): %f ms\n", time_step);
        }

        // Swap old and new centroids (Step 2.6)
        swap(new_centroids, old_centroids);

        real l1_thresh = opts.threshold / opts.dimensions;

        // Check convergence (Step 2.7)
        // 6. 計算質心的收斂程度
        bool converged = transform_reduce(
            make_zip_iterator(make_tuple(new_centroids.begin(), old_centroids.begin())),
            make_zip_iterator(make_tuple(new_centroids.end(), old_centroids.end())),
            check_convergence_thrust_op(l1_thresh),
            true,
            logical_and<bool>()
        );

        iterations++;
        done = (iterations > opts.max_iterations) || converged;
    }

    // Step 3: Copy final results back to host
    // 計算從設備到主機的傳輸時間 Calculate transfer time from device to host
    cudaEventRecord(start_transfer, 0);
    copy(old_centroids.begin(), old_centroids.end(), centroids);
    copy(point_to_centroid_map.begin(), point_to_centroid_map.end(), point_cluster_ids);
    cudaEventRecord(stop_transfer, 0);
    cudaEventSynchronize(stop_transfer);

    float device_to_host_transfer_time;
    cudaEventElapsedTime(&device_to_host_transfer_time, start_transfer, stop_transfer);
    transfer_time += device_to_host_transfer_time;

    // Final timing
    cudaEventRecord(stop_total, 0);
    cudaEventSynchronize(stop_total);

    // Calculate total runtime
    cudaEventElapsedTime(&total_runtime, start_total, stop_total);
    per_iteration_time = total_runtime / iterations;
    float time_taken = total_runtime - transfer_time;

    // Cleanup memory and events
    cudaEventDestroy(start_transfer);
    cudaEventDestroy(stop_transfer);
    cudaEventDestroy(start_total);
    cudaEventDestroy(stop_total);
    cudaEventDestroy(start_iter);
    cudaEventDestroy(stop_iter);

    // Print debug information
    if (opts.debug || PERFORMANCE_TEST) {
        std::cout << "Total runtime: " << total_runtime << " ms" << std::endl;
        std::cout << "Total data transfer time: " << transfer_time << " ms" << std::endl;
        std::cout << "Percent spent in IO: " << transfer_time/total_runtime << " ms" << std::endl;
        std::cout << "Fraction of time spent on data transfer: " << (transfer_time / total_runtime) << std::endl;
        std::cout << "Algorithm Time taken by function: " << time_taken << " ms" << std::endl;
    }

    per_iteration_time = time_taken / iterations;
    print_ExecuteTime(iterations, per_iteration_time, time_taken);
    return iterations;
}