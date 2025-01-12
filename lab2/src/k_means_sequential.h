#pragma once
#include "common.h"  // 引入 common.h 以獲取 real 型態
#include "argparse.h"

int k_means_sequential(int n_points, real* data_points, struct options_t opts, int* point_cluster_ids, real* centroids, double& per_iteration_time);
