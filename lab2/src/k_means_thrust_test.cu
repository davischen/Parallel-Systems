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

typedef thrust::tuple<real, int> real_indexed;

#define POW2(x) ((x) * (x))

// 定義 compute_distances 核心函數
__global__ void compute_distances(
    const real* points, const real* centroids, real* distances,
    int dims, int n_points, int n_clusters) {
    
    int point_id = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (point_id < n_points) {
        for (int j = 0; j < n_clusters; j++) {
            real distance = 0.0;
            for (int d = 0; d < dims; d++) {
                real diff = points[point_id * dims + d] - centroids[j * dims + d];
                distance += diff * diff;
            }
            distances[point_id * n_clusters + j] = distance;
        }
    }
}

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

struct distance_component : public thrust::unary_function<int, real> {
    const int d, k;
    const real* points, * centroids;

    distance_component(int _d, int _k, real* _points, real* _centroids)
        : d(_d), k(_k), points(_points), centroids(_centroids) {}

    __host__ __device__
    real operator()(int index) {
        int i = (index / d) / k;
        int j = (index / d) % k;
        int l = index % d;
        return POW2(points[i * d + l] - centroids[j * d + l]);
    }
};

struct updateCentroidSums_thrust : public thrust::unary_function<void, thrust::tuple<real, int>> {
    const int d;
    const int* d_point_cluster_ids;
    real* centroids;

    updateCentroidSums_thrust(int _d, int* _d_point_cluster_ids, real* _centroids)
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

struct computeCentroidAverages_thrust : public thrust::unary_function<void, thrust::tuple<int, int>> {
    const int d, i;
    real* centroids;

    computeCentroidAverages_thrust(int _d, int _i, real* _centroids)
        : d(_d), i(_i), centroids(_centroids) {}

    __device__
    void operator()(thrust::tuple<int, int> ints) {
        int centroid_id = thrust::get<0>(ints);
        int counts = thrust::get<1>(ints);
        *(centroids + centroid_id * d + i) /= counts;
    }
};

struct l1_op : public thrust::unary_function<bool, thrust::tuple<real, real>> {
    const real l1_thresh;
    l1_op(real _l1_thresh) : l1_thresh(_l1_thresh) {}

    __host__ __device__
    bool operator()(thrust::tuple<real, real> realz) {
        real x1 = thrust::get<0>(realz);
        real x2 = thrust::get<1>(realz);
        return abs(x1 - x2) < l1_thresh;
    }
};

int k_means_thrust_test(int n_points, real* data_points, struct options_t opts, int* point_cluster_ids, real* centroids, double& per_iteration_time) {
    using namespace thrust;
    using namespace thrust::placeholders;

    bool done = false;
    int iterations = 0;

    // CUDA events for timing
    cudaEvent_t start_total, stop_total, start_transfer, stop_transfer;
    cudaEvent_t start_iter, stop_iter;
    float transfer_time = 0.0, total_runtime = 0.0;
    cudaEventCreate(&start_total);
    cudaEventCreate(&stop_total);
    cudaEventCreate(&start_transfer);
    cudaEventCreate(&stop_transfer);
    cudaEventCreate(&start_iter);
    cudaEventCreate(&stop_iter);

    cudaEventRecord(start_total, 0);  // Start overall timing

    // 設置 block 和 thread 的大小
    int threadsPerBlock = 1024;
    int blocksPerGrid = (n_points + threadsPerBlock - 1) / threadsPerBlock;

    // 將資料從主機（CPU）傳輸到設備（GPU） (Step 1)
    cudaEventRecord(start_transfer, 0);
    dv_real d_points(data_points, data_points + n_points * opts.dimensions);
    dv_real old_centroids(centroids, centroids + opts.n_clusters * opts.dimensions);

    cudaEventRecord(stop_transfer, 0);
    cudaEventSynchronize(stop_transfer);

    // 計算從主機到設備的傳輸時間
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
    while (!done) {
        cudaEventRecord(start_iter, 0);

        // Compute distance components (Step 2.1)
        distance_component dist_comp(opts.dimensions, opts.n_clusters, raw_pointer_cast(d_points.data()), raw_pointer_cast(old_centroids.data()));

        // Launch custom kernel for computing distances
        compute_distances<<<blocksPerGrid, threadsPerBlock>>>(
            raw_pointer_cast(d_points.data()), raw_pointer_cast(old_centroids.data()),
            raw_pointer_cast(point_centroid_distances.data()), opts.dimensions, n_points, opts.n_clusters
        );

        if (PERFORMANCE_TEST) {
            cudaEventRecord(stop_iter, 0);
            cudaEventSynchronize(stop_iter);
            float step_time;
            cudaEventElapsedTime(&step_time, start_iter, stop_iter);
            printf("Step 2.1 (Distance computation): %f ms\n", step_time);
            cudaEventRecord(start_iter);
        }

        // Assign points to centroids (Step 2.2)
        reduce_by_key(
            thrust::cuda::par.on(0),
            make_transform_iterator(counting_iterator<int>(0), _1 / opts.n_clusters),
            make_transform_iterator(counting_iterator<int>(n_points * opts.n_clusters), _1 / opts.n_clusters),
            make_zip_iterator(make_tuple(point_centroid_distances.begin(), counting_iterator<int>(0))),
            make_discard_iterator(),
            make_zip_iterator(make_tuple(make_discard_iterator(), point_to_centroid_map.begin())),
            equal_to<int>(),
            maximum_by_first()
        );

        transform(
            thrust::cuda::par.on(0),
            point_to_centroid_map.begin(),
            point_to_centroid_map.end(),
            point_to_centroid_map.begin(),
            _1 % opts.n_clusters
        );

        if (PERFORMANCE_TEST) {
            cudaEventRecord(stop_iter, 0);
            cudaEventSynchronize(stop_iter);
            float step_time;
            cudaEventElapsedTime(&step_time, start_iter, stop_iter);
            printf("Step 2.2 (Assign points to centroids): %f ms\n", step_time);
            cudaEventRecord(start_iter);
        }

        fill(thrust::cuda::par.on(0), new_centroids.begin(), new_centroids.end(), 0);
        updateCentroidSums_thrust compute_assign_means(opts.dimensions, raw_pointer_cast(point_to_centroid_map.data()), raw_pointer_cast(new_centroids.data()));
        for_each(
            thrust::cuda::par.on(0),
            make_zip_iterator(make_tuple(d_points.begin(), counting_iterator<int>(0))),
            make_zip_iterator(make_tuple(d_points.end(), counting_iterator<int>(n_points * opts.dimensions))),
            compute_assign_means
        );

        if (PERFORMANCE_TEST) {
            cudaEventRecord(stop_iter, 0);
            cudaEventSynchronize(stop_iter);
            float step_time;
            cudaEventElapsedTime(&step_time, start_iter, stop_iter);
            printf("Step 2.3 (Compute new centroids): %f ms\n", step_time);
            cudaEventRecord(start_iter);
        }

        d_point_cluster_ids = point_to_centroid_map;
        sort(thrust::cuda::par.on(0), d_point_cluster_ids.begin(), d_point_cluster_ids.end());
        auto new_end = reduce_by_key(
            thrust::cuda::par.on(0),
            d_point_cluster_ids.begin(),
            d_point_cluster_ids.end(),
            make_constant_iterator(1),
            d_k_count_keys.begin(),
            d_k_counts.begin()
        );

        fill(thrust::cuda::par.on(0), new_end.second, d_k_counts.end(), 0);

        if (PERFORMANCE_TEST) {
            cudaEventRecord(stop_iter, 0);
            cudaEventSynchronize(stop_iter);
            float step_time;
            cudaEventElapsedTime(&step_time, start_iter, stop_iter);
            printf("Step 2.4 (Sort cluster IDs and reduce): %f ms\n", step_time);
            cudaEventRecord(start_iter);
        }

        for (int i = 0; i < opts.dimensions; i++) {
            for_each(
                thrust::cuda::par.on(0),
                make_zip_iterator(make_tuple(d_k_count_keys.begin(), d_k_counts.begin())),
                make_zip_iterator(make_tuple(new_end.first, new_end.second)),
                computeCentroidAverages_thrust(opts.dimensions, i, raw_pointer_cast(new_centroids.data()))
            );
        }

        if (PERFORMANCE_TEST) {
            cudaEventRecord(stop_iter, 0);
            cudaEventSynchronize(stop_iter);
            float step_time;
            cudaEventElapsedTime(&step_time, start_iter, stop_iter);
            printf("Step 2.5 (Divide centroids by counts): %f ms\n", step_time);
        }

        swap(new_centroids, old_centroids);
        real l1_thresh = opts.threshold / opts.dimensions;
        bool converged = transform_reduce(
            thrust::cuda::par.on(0),
            make_zip_iterator(make_tuple(new_centroids.begin(), old_centroids.begin())),
            make_zip_iterator(make_tuple(new_centroids.end(), old_centroids.end())),
            l1_op(l1_thresh),
            true,
            logical_and<bool>()
        );

        iterations++;
        done = (iterations > opts.max_iterations) || converged;
    }

    cudaEventRecord(start_transfer, 0);
    copy(thrust::cuda::par.on(0), old_centroids.begin(), old_centroids.end(), centroids);
    copy(thrust::cuda::par.on(0), point_to_centroid_map.begin(), point_to_centroid_map.end(), point_cluster_ids);
    cudaEventRecord(stop_transfer, 0);
    cudaEventSynchronize(stop_transfer);

    float device_to_host_transfer_time;
    cudaEventElapsedTime(&device_to_host_transfer_time, start_transfer, stop_transfer);
    transfer_time += device_to_host_transfer_time;

    cudaEventRecord(stop_total, 0);
    cudaEventSynchronize(stop_total);
    cudaEventElapsedTime(&total_runtime, start_total, stop_total);
    per_iteration_time = total_runtime / iterations;

    cudaEventDestroy(start_transfer);
    cudaEventDestroy(stop_transfer);
    cudaEventDestroy(start_total);
    cudaEventDestroy(stop_total);
    cudaEventDestroy(start_iter);
    cudaEventDestroy(stop_iter);

    if (opts.debug || PERFORMANCE_TEST) {
        std::cout << "Total runtime: " << total_runtime << " ms" << std::endl;
        std::cout << "Total data transfer time: " << transfer_time << " ms" << std::endl;
        std::cout << "Fraction of time spent on data transfer: " << (transfer_time / total_runtime) << std::endl;
    }
    print_ExecuteTime(iterations, per_iteration_time,total_runtime);
    return iterations;
}
