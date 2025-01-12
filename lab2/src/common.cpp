#include "common.h"
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include "argparse.h"

#include <iostream>

static unsigned long int next = 1;
static unsigned long kmeans_rmax = 32767;
int k_means_rand() {
  next = next * 1103515245 + 12345;
  return (unsigned int)(next/65536) % (kmeans_rmax+1);
}

void k_means_srand(unsigned int seed) {
  next = seed;
}

void init_kmeans_centroids(int n_points, real *points,
    real *centroids, struct options_t opts) {
  k_means_srand(opts.seed); // cmd_seed is a cmdline arg

  for (int i = 0; i < opts.n_clusters; i++) {
    int index = k_means_rand() % n_points;
    std::memcpy(&centroids[i * opts.dimensions], &points[index * opts.dimensions], opts.dimensions * sizeof(real));
  }
}

// Check correctness by ensuring centroids are within threshold
bool check_correctness(int n_clusters, int dims, const real *centroids_old, const real *centroids_new, real threshold) {
    for (int i = 0; i < n_clusters * dims; i++) {
        if (std::abs(centroids_old[i] - centroids_new[i]) > threshold) {
            return false;
        }
    }
    return true;
}

// Define the function
void check_and_print_results(int n_clusters, int dimensions, 
                             const real* centroids_old, const real* centroids_new, 
                             double threshold) 
{
        // Check correctness using threshold as epsilon
        bool correct = check_correctness(n_clusters, dimensions, centroids_old, centroids_new, threshold);
        
        if (correct) {
            std::cout << "Results are correct within threshold " << threshold << std::endl;
        } else {
            std::cout << "Results are NOT correct within threshold " << threshold << std::endl;
        }
}


// 初始化隨機質心
// 初始化隨機質心的函數
// 這個函數會從給定的資料集中隨機選擇點，並將其作為初始質心
// n_points: 資料集中的點的數量
// dimensions: 每個資料點的維度數
// points: 資料集中所有點的數組（每個點的維度數據平鋪存放）
// n_clusters: 要生成的聚類數（即質心數量）
// centroids: 用來存放隨機生成的質心數組
// seed: 隨機數生成器的種子，用來保證每次執行隨機選擇的結果相同（如果提供相同的 seed）
void init_random_centroids(int n_clusters, int dimensions, int n_points, const real *points, real *centroids, int seed) {
    // 設置隨機數生成器的種子，使隨機選擇的結果可重現
    std::srand(seed);
    
    // 循環遍歷每個要初始化的質心
    for (int i = 0; i < n_clusters; i++) {
        // 隨機選擇一個資料點作為質心
        int random_index = std::rand() % n_points;
        
        // 將選定的資料點的所有維度數據複製到對應的質心數組中
        // 這裡使用了一個循環來處理該點的所有維度
        for (int d = 0; d < dimensions; d++) {
            // 將資料點的第 d 維度的值複製到第 i 個質心的第 d 維度
            // centroids[i * dimensions + d] 表示第 i 個質心的第 d 維位置
            // points[random_index * dimensions + d] 表示選擇的資料點的第 d 維數據
            centroids[i * dimensions + d] = points[random_index * dimensions + d];
        }
    }
}

// 函數：輸出最終簇的質心
void print_Centroids(int _k, int _d, double* centroids) {
    for (int clusterId = 0; clusterId < _k; clusterId++) {
        printf("%d ", clusterId);  
        for (int d = 0; d < _d; d++) {
            printf("%lf ", centroids[(clusterId * _d) + d]);  
        }
        printf("\n"); 
    }
}

// 函數：輸出每個點的最終簇ID
void print_ClusterAssignments(int nPoints, int* clusterId_of_point) {
    printf("clusters:"); 
    for (int p = 0; p < nPoints; p++) {
        printf(" %d", clusterId_of_point[p]); 
    }
    printf("\n"); 
}

// 修正後的 print_centroids 函數，輸出格式符合規範
/*void print_centroids(const real* centroids, int dims, int n_clusters) {
    for (int i = 0; i < n_clusters; i++) {
        // 輸出 clusterId
        printf("%d ", i);
        // 輸出該質心的每個維度值
        for (int d = 0; d < dims; ++d) {
            printf("%lf ", centroids[i * dims + d]);
            //if ((d+1) != opts->nDims) printf(" ");
        }
        printf("\n");
    }
}*/

// 修正後的 print_cluster_assignments 函數，輸出格式符合規範
/*void print_cluster_ids(int nPoints, const int* clusterId_of_point) {
    // 輸出 clusters: 並列出每個點的群集 ID
    printf("clusters:");
    for (int p=0; p < nPoints; p++)
        printf(" %d", clusterId_of_point[p]);

}*/

void print_ExecuteTime(int iter_to_converge, double time_per_iter_in_ms,double time_taken) {
    //Algorithm time, iter_to_converge, Per iteration time, 記得修改%d,%lf\n
    //printf("%0.4f,%d,%0.4f\n", time_taken,iter_to_converge, time_per_iter_in_ms);
    //printf("%d,%lf\n", iter_to_converge, time_per_iter_in_ms);
    printf("%d,%lf\n", iter_to_converge, time_per_iter_in_ms);
}

// 打印每個聚類的標籤數量，用於檢查點被分配到的各個聚類中的情況
void print_ClusterLabelCount(int* counts, int k) {
    printf("print cluster label count: ");
    for (int i = 0; i < k; i++){
        printf(" %d", counts[i]);
    }
    printf("\n");
}

// 打印每個聚類的收斂值，用於檢查收斂情況
void print_ConvergenceSum(double* convergence_k, int k) {
    printf("print convergence sum: ");
    for (int i = 0; i < k; i++){
        printf(" %lf", convergence_k[i]);
    }
    printf("\n");
}

//打印質心位置 (printCenters)
/*void print_Centers(real* centers, int k, int dims) {
    printf("Centroid positions:\n");
    for (int i = 0; i < k; i++) {
        printf("Centroid %d: ", i);
        for (int d = 0; d < dims; d++) {
            printf("%lf ", centers[i * dims + d]);
        }
        printf("\n");
    }
}*/