#include "parallel_mpi.h"
#include <chrono>
#include <cmath>
#include <mpi.h>
#include <iostream>
#include <cstring>  // 使用 memset 初始化內存
#include <array>
#include "io.h"
#include "tree.h"

// 定義 MPI 粒子類型
/*void define_particle_mpi_type(MPI_Datatype* particle_mpi_type) {
    // 設定粒子的結構信息
    MPI_Datatype types[] = {MPI_INT, MPI_DOUBLE};  // 數據類型：整數和浮點數
    int block_lengths[] = {1, 7};  // 分別對應 index 和 7 個浮點數字段的長度
    MPI_Aint displacements[] = {
        offsetof(particle, index),  // index 偏移
        offsetof(particle, x)       // x 偏移，代表粒子的其餘數據起點
    };

    // 創建自定義 MPI 數據類型
    MPI_Type_create_struct(2, block_lengths, displacements, types, particle_mpi_type);
    MPI_Type_commit(particle_mpi_type);  // 提交數據類型到 MPI
}*/


MPI_Datatype mpiBody;
extern MPI_Datatype mpiBody;

void initializeMPITypes(){
    int blocklengths[] = {1, 1,1, 1,1, 1,1,1};
    MPI_Datatype types[] = {MPI_INT, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE,MPI_DOUBLE, MPI_DOUBLE,MPI_DOUBLE, MPI_DOUBLE};
    MPI_Aint offsets[8];

    offsets[0] = offsetof(struct particle, index);
    offsets[1] = offsetof(struct particle, x);
    offsets[2] = offsetof(struct particle, y);
    offsets[3] = offsetof(struct particle, v_x);
    offsets[4] = offsetof(struct particle, v_y);
    offsets[5] = offsetof(struct particle, mass);
    offsets[6] = offsetof(struct particle, a_x);
    offsets[7] = offsetof(struct particle, a_y);

    MPI_Type_create_struct(8, blocklengths, offsets, types, &mpiBody);
    MPI_Type_commit(&mpiBody);
}

void freeMPITypes(){
    MPI_Type_free(&mpiBody);
}


MPI_Datatype define_particle_type() {
    MPI_Datatype particle_mpi_type;
    MPI_Aint offsets[] = {
        offsetof(particle, index),
        offsetof(particle, x),
        offsetof(particle, y),
        offsetof(particle, mass),
        offsetof(particle, v_x),
        offsetof(particle, v_y),
        offsetof(particle, a_x),
        offsetof(particle, a_y)
    };
    int block_lengths[] = {1, 1, 1, 1, 1, 1, 1, 1};
    MPI_Datatype types[] = {
        MPI_INT, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE,
        MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE
    };

    MPI_Type_create_struct(8, block_lengths, offsets, types, &particle_mpi_type);
    MPI_Type_commit(&particle_mpi_type);

    return particle_mpi_type;
}


int parallel_mpi(options_t* opts, int rank, int num_procs) {

    //struct options_t opts;
    double dt;
    double start_time, stop_time;
    struct particle *bodies = NULL;
    struct Node *root;
    int s,i,c;
    struct particle *tempBodies = NULL;

    //get_opts(argc, argv, &opts);

/*#ifdef DEBUG
    DEBUG_PRINT(std::cout << "---------------BHParallel------------------" << std::endl);
    DEBUG_PRINT(std::cout << "Input file: " << opts->in_file << std::endl);
    DEBUG_PRINT(std::cout << "Output file: " << opts->out_file << std::endl);
    DEBUG_PRINT(std::cout << "Number of steps: " << opts->n_steps << std::endl);
    DEBUG_PRINT(std::cout << "Threshold: " << opts->threshold << std::endl);
    DEBUG_PRINT(std::cout << "Timestep: " << opts->timestep << std::endl);
    DEBUG_PRINT(std::cout << "Visualization: " << (opts->visualization ? "Enabled" : "Disabled") << std::endl);
    DEBUG_PRINT(std::cout << "Sequential Mode: " << (opts->sequential ? "Enabled" : "Disabled") << std::endl);
    DEBUG_PRINT(std::cout << "---------------------------------" << std::endl);
#endif*/
    //DEBUG_PRINT(std::cout << "Reading particle data from file: " << opts->in_file << std::endl);
    
    read_file_parallel(opts, &bodies,num_procs);

    dt = opts->timestep;

    int subGrps = opts->n_bodiesParallel/num_procs;

    initializeMPITypes();
    MPI_Barrier(MPI_COMM_WORLD);
    start_time = MPI_Wtime();

    for (s = 0; s < opts->n_steps; s++) {

        tempBodies = &bodies[(rank*subGrps)];

        root = (struct Node *)malloc(sizeof(struct Node));
        memset((char *)root, 0, sizeof(struct Node));

        initialize_root(root);

        for (int i = 0; i < opts->n_particles; ++i) {
            insertBody(opts, root, &bodies[i]);
        }

        /* Compute forces */
        std::vector<std::array<double, 2>> forces(subGrps, std::array<double, 2>{0, 0});
        //std::vector<array<double, 2>> forces(subGrps, {0,0});
        for(i = 0; i < subGrps; i++) {
            struct particle *body = &tempBodies[i];
            if (body->index == -10) continue;
            forces[i]= compute_force_v2(opts, root, body);
        }

        /* Update positions */
        for(i = 0; i < subGrps; i++) {
            struct particle *body = &tempBodies[i];
            if (body->index == -10) continue;
            updateParticleState(body, dt, root);
        }

        MPI_Allgather(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, bodies, subGrps, mpiBody, MPI_COMM_WORLD);

        tearDownTree(root);
    }

    stop_time = MPI_Wtime();

    if (rank == 0) {
        printf("%f\n", (stop_time-start_time));
        write_file_parallel(opts, bodies);
    }

    if (bodies)
        free(bodies);

    /* Finalize */
    freeMPITypes();
    MPI_Finalize();

    return 0;
}

int parallel_mpi_send_recv(options_t* opts, int rank, int num_procs) {
    double dt;
    double start_time, stop_time;
    struct particle *bodies = NULL;
    struct Node *root;
    int s, i;
    struct particle *tempBodies = NULL;

    read_file_parallel(opts, &bodies, num_procs);

    dt = opts->timestep;
    int subGrps = opts->n_bodiesParallel / num_procs;

    initializeMPITypes();
    MPI_Barrier(MPI_COMM_WORLD);
    start_time = MPI_Wtime();

    for (s = 0; s < opts->n_steps; s++) {
        tempBodies = &bodies[rank * subGrps];

        root = (struct Node *)malloc(sizeof(struct Node));
        memset((char *)root, 0, sizeof(struct Node));

        initialize_root(root);

        // Insert bodies into the tree
        for (int i = 0; i < opts->n_particles; ++i) {
            insertBody(opts, root, &bodies[i]);
        }

        // Compute forces
        std::vector<std::array<double, 2>> forces(subGrps, std::array<double, 2>{0, 0});
        for (i = 0; i < subGrps; i++) {
            struct particle *body = &tempBodies[i];
            if (body->index == -10) continue;
            forces[i] = compute_force_v2(opts, root, body);
        }

        // Update positions
        for (i = 0; i < subGrps; i++) {
            struct particle *body = &tempBodies[i];
            if (body->index == -10) continue;
            updateParticleState(body, dt, root);
        }

        // Rank 0 gathers updated data from all processes
        if (rank == 0) {
            for (int p = 1; p < num_procs; p++) {
                MPI_Recv(&bodies[p * subGrps], subGrps, mpiBody, p, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        } else {
            // Each process sends its updated data to Rank 0
            MPI_Send(tempBodies, subGrps, mpiBody, 0, 0, MPI_COMM_WORLD);
        }

        // Broadcast updated bodies from Rank 0 to all processes
        MPI_Bcast(bodies, opts->n_bodiesParallel, mpiBody, 0, MPI_COMM_WORLD);

        tearDownTree(root);
    }

    stop_time = MPI_Wtime();

    if (rank == 0) {
        printf("%f\n", (stop_time - start_time));
        write_file_parallel(opts, bodies);
    }

    if (bodies)
        free(bodies);

    freeMPITypes();
    MPI_Finalize();

    return 0;
}

/*
int parallel_mpi_send_recv_optimize(options_t* opts, int rank, int num_procs) {
    double dt = opts->timestep;
    double start_time, stop_time;

    struct particle* bodies = nullptr;
    int n_particles = 0;

    // 初始化 MPI 類型
    MPI_Datatype particle_mpi_type = define_particle_type();

    // Rank 0 讀取資料並廣播粒子數
    if (rank == 0) {
        read_file(opts, &n_particles, &bodies);
    }
    MPI_Bcast(&n_particles, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // 非 Rank 0 分配記憶體
    if (rank != 0) {
        bodies = (particle*)malloc(n_particles * sizeof(particle));
    }
    MPI_Bcast(bodies, n_particles, particle_mpi_type, 0, MPI_COMM_WORLD);

    // 靜態分區
    int partition_size = (n_particles + num_procs - 1) / num_procs;
    int start_idx = rank * partition_size;
    int end_idx = std::min(start_idx + partition_size, n_particles);

    MPI_Barrier(MPI_COMM_WORLD);
    start_time = MPI_Wtime();

    for (int s = 0; s < opts->n_steps; ++s) {
        // 建立四叉樹
        struct Node* root = (struct Node*)malloc(sizeof(struct Node));
        memset((char*)root, 0, sizeof(struct Node));
        initialize_root(root);

        // 插入粒子到樹中
        for (int i = start_idx; i < end_idx; ++i) {
            insertBody(opts, root, &bodies[i]);
        }

        // 計算力
        std::vector<std::array<double, 2>> forces(partition_size, {0.0, 0.0});
        for (int i = start_idx; i < end_idx; ++i) {
            forces[i - start_idx] = compute_force_v2(opts, root, &bodies[i]);
        }

        // 更新粒子狀態
        for (int i = start_idx; i < end_idx; ++i) {
            updateParticleState(&bodies[i], dt, root);
        }

        // Rank 0 收集數據
        if (rank == 0) {
            for (int p = 1; p < num_procs; ++p) {
                MPI_Recv(
                    &bodies[p * partition_size],
                    partition_size,
                    particle_mpi_type,
                    p,
                    0,
                    MPI_COMM_WORLD,
                    MPI_STATUS_IGNORE
                );
            }
        } else {
            MPI_Send(
                &bodies[start_idx],
                partition_size,
                particle_mpi_type,
                0,
                0,
                MPI_COMM_WORLD
            );
        }

        // 廣播更新後的粒子數據
        MPI_Bcast(bodies, n_particles, particle_mpi_type, 0, MPI_COMM_WORLD);

        // 清理樹
        tearDownTree(root);
    }

    stop_time = MPI_Wtime();

    // Rank 0 寫出結果
    if (rank == 0) {
        printf("%lf\n", stop_time - start_time);
        write_file(opts, n_particles, bodies);
    }

    // 清理記憶體
    if (bodies) free(bodies);
    MPI_Type_free(&particle_mpi_type);
    MPI_Finalize();

    return 0;
}
*/