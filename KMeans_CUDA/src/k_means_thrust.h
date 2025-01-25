#pragma once
#include "common.h"  // 獲取 real 型別定義
#include "argparse.h"

// 使用 Thrust 的 K-means 實現
int k_means_thrust_optimized(int n_points, real *data_points, struct options_t opts, int *point_cluster_ids, real *centroids, double &per_iteration_time);

int k_means_thrust_test(int n_points, real *data_points, struct options_t opts, int *point_cluster_ids, real *centroids, double &per_iteration_time);