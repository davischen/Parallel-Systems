// common.h
#pragma once

// 定義 real 作為 float 或 double 的別名
typedef double real;  // 可以根據需要改成 float

void init_random_centroids(int n_clusters, int dimensions, int n_points, const real* points, real* centroids, int seed);

int k_means_rand();

void init_kmeans_centroids(int n_points, real *points,
    real *centroids, struct options_t opts);

void check_and_print_results(int n_clusters, int dimensions, 
                             const real* centroids_old, const real* centroids_new, 
                             double threshold);

void print_Centroids(int _k, int _d, double* centroids);
void print_ClusterAssignments(int nPoints, int* clusterId_of_point);
void print_ClusterLabelCount(int* counts, int k);
void print_ConvergenceSum(double* convergence_k, int k);
void print_Centers(real* centers, int k, int dims);
void print_ExecuteTime(int iterations, double per_iteration_time,double time_taken);

#define DEBUG_TEST 0
#define PERFORMANCE_TEST 0

//#define DEBUG_OUT(x) { if(DEBUG_TEST) std::cerr << x << std::endl; }
//#define TIMING_PRINT(x) do { if (TIMING_TEST) { printf("TIMING: "); x;} } while (0)
