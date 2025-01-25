#pragma once
#include "common.h"  // 獲取 real 型別定義
#include "argparse.h"

// 使用 CUDA 共享記憶體優化的 K-means 實現
int k_means_cuda_shared(int n_points, real* data_points, struct options_t opts, int* point_cluster_ids, real* centroids, double& per_iteration_time);
