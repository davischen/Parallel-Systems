#pragma once

#include "common.h"
#include "particle.h"
#include "argparse.h"  // 確保包含 options_t 定義

// 函數聲明
void read_file(const options_t* args, int* numParticles, particle** particles);

void write_file(const options_t* args, int numParticles, const particle* particles);

void read_file_parallel(struct options_t* opts, struct particle **bodies, int size);

void write_file_parallel(struct options_t* opts,
                struct particle *bodies);