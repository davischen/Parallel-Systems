#ifndef VISUALIZATION_H
#define VISUALIZATION_H
#include "tree.h"

// 初始化視覺化窗口

bool render_visualization(int, particle const*, Node const*, int);
void terminate_visualization();
int init_visualization();
#endif // VISUALIZATION_H
