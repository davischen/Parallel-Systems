#ifndef BARNES_HUT_MPI_H
#define BARNES_HUT_MPI_H

#include "argparse.h"  // 假設 options_t 的定義在此檔案中
#include "particle.h"
#include <mpi.h>
// Barnes-Hut MPI 版本主函數

int parallel_mpi(options_t* opts, int rank, int size);
int parallel_mpi_send_recv(options_t* opts, int rank, int num_procs);

#endif  // BARNES_HUT_MPI_H
