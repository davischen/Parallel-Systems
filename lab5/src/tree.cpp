#include "tree.h"
#include <cmath>
#include <cstring>
#include "particle.h"
#include <iostream> // 添加打印功能
#include <array>
#include "common.h"

// 更新粒子狀態

void updateParticleState(particle* body, double timestep, const Node* root) {
    body->x += body->v_x * timestep + 0.5 * body->a_x * timestep * timestep;
    body->y += body->v_y * timestep + 0.5 * body->a_y * timestep * timestep;
    body->v_x += body->a_x * timestep;//(force[0] / body->mass) * timestep;
    body->v_y += body->a_y * timestep;//(force[1] / body->mass) * timestep;
    // 檢查是否超出邊界
    if (!contains(root, body)) {
        body->mass = OUT_OF_BOUNDS_MASS;
    }
}

//原作版本
void updateParticleState_v2(particle* body, const std::array<double, 2>& force, double timestep, const Node* root) {
    body->x += body->v_x * timestep + 0.5 * (force[0] / body->mass) * timestep * timestep;
    body->y += body->v_y * timestep + 0.5 * (force[1] / body->mass) * timestep * timestep;
    body->v_x += (force[0] / body->mass) * timestep;
    body->v_y += (force[1] / body->mass) * timestep;
    /*for(int c = 0; c < 2; c++) {
                body->pos[c] = body->pos[c] + body->vel[c] * timestep + 0.5 * (force[c]/body->mass) * pow(timestep,2);
                body->vel[c] = body->vel[c] + (force[c]/body->mass) * timestep;
            }
            if (!contains(root, b)) {
                body->mass = -1;
    }*/
    // 檢查是否超出邊界
    if (!contains(root, body)) {
        body->mass = OUT_OF_BOUNDS_MASS;
    }
}
//原作版本
std::array<double, 2> compute_force_v2(const options_t* opts, const Node* node, particle* body) {
    std::array<double, 2> f = {0, 0};
    double norm[2];
    const double G = 0.0001;

    // 打印粒子狀態
    /*std::cout << "Computing force for Particle " << body->index
              << " (x=" << body->x << ", y=" << body->y
              << ", mass=" << body->mass << ")" << std::endl;*/

    if (node && node->data && body->mass != -1) {
        if (node->data->index != body->index) {
            norm[0] = node->data->x - body->x;
            norm[1] = node->data->y - body->y;
            double d = sqrt(norm[0] * norm[0] + norm[1] * norm[1]);
            double limited_dist = d >= 0.03 ? d : 0.03;

            if (!node->has_children || ((double)node->s / d < opts->threshold)) {
                for (int i = 0; i < 2; i++) {
                    f[i] = ((G * (node->data->mass * body->mass)) / (limited_dist * limited_dist)) * (norm[i] / d);
                }
                /*std::cout << "Force on Particle " << body->index
                          << " from Node: f_x=" << f[0] << ", f_y=" << f[1] << std::endl;*/
                return f;
            }
        } else {
            //std::cout << "Particle " << body->index << " is the same as the node particle, skipping." << std::endl;
            return {{0, 0}};
        }
    } else {
        if (!node) {
            //std::cout << "Node is null, skipping." << std::endl;
        }
        return {{0, 0}};
    }

    std::array<double, 2> NEf = {0, 0};
    std::array<double, 2> NWf = {0, 0};
    std::array<double, 2> SEf = {0, 0};
    std::array<double, 2> SWf = {0, 0};

    // 遞迴處理子節點
    if (node->has_children) {
        if (node->chd[0]) {
            NEf = compute_force_v2(opts, node->chd[0], body);
        }
        if (node->chd[1]) {
            NWf = compute_force_v2(opts, node->chd[1], body);
        }
        if (node->chd[2]) {
            SEf = compute_force_v2(opts, node->chd[2], body);
        }
        if (node->chd[3]) {
            SWf = compute_force_v2(opts, node->chd[3], body);
        }
    }

    // 合併所有方向的力
    for (int i = 0; i < 2; i++) {
        f[i] = NEf[i] + NWf[i] + SEf[i] + SWf[i];
    }
    // 更新粒子的加速度
    body->a_x = f[0] / body->mass;
    body->a_y = f[1] / body->mass;
    // 打印最終力
    /*std::cout << "Total force on Particle " << body->index
              << ": f_x=" << f[0] << ", f_y=" << f[1] << std::endl;*/

    return f;
}

// 檢查粒子是否位於節點內
bool contains(const Node* node, const particle* p) {
    bool result = (p->x >= node->min_bound[0] && p->x <= node->max_bound[0] &&
                   p->y >= node->min_bound[1] && p->y <= node->max_bound[1]);
    //std::cout << "Particle " << p->index << (result ? " is " : " is not ") << "contained in the node." << std::endl;
    return result;
}

// 釋放四叉樹資源
void tearDownTree(Node* node) {
    if (node) {
        for (int i = 0; i < 4; ++i) {
            if (node->chd[i]) {
                tearDownTree(node->chd[i]); // 遞歸釋放子節點
                node->chd[i] = nullptr;
            }
        }
        if (node->has_children && node->data && node->bIdx == -1) {
            free(node->data); // 如果是虛擬粒子，釋放內存
        }
        free(node); // 釋放節點本身
    }
}

// 初始化根節點
void initialize_root(Node* root) {
    memset(root, 0, sizeof(Node)); // 初始化節點內存
    root->min_bound[0] = 0.0;
    root->min_bound[1] = 0.0;
    root->max_bound[0] = 4.0;
    root->max_bound[1] = 4.0;
    root->s = 4.0; // 根節點的尺寸
    root->has_children = false; // 標記根節點未被細分
    root->data = nullptr; // 根節點暫無粒子數據
    root->parent = NULL;
    root->bIdx = 0;
}

Node* createNode(double minX, double minY, double maxX, double maxY, double size, Node* parent) {
    Node* node = (Node*)malloc(sizeof(Node));
    node->min_bound[0] = minX;
    node->min_bound[1] = minY;
    node->max_bound[0] = maxX;
    node->max_bound[1] = maxY;
    node->s = size;
    node->has_children = false;
    node->data = nullptr;
    node->parent = parent;
    for (int i = 0; i < 4; i++) {
        node->chd[i] = nullptr;
    }
    return node;
}

//自己的改良版
//DFS方式插入
void insertBody(const options_t* opts, Node* node, particle* p) {
    if (!node || !p) return; // 無效節點或粒子，直接返回

    // 如果節點尚未分裂且無數據，直接插入粒子
    if (node->data == nullptr && !node->has_children) {
        node->data = p;
        node->bIdx = p->index;
        return;
    }

    // 如果節點尚未分裂，進行分裂
    if (!node->has_children) {
        splitNode(node); // 分裂節點（參見下面的 splitNode 函數）

        // 將已有粒子重新插入到正確的子節點
        particle* temp = node->data;
        for (int i = 0; i < 4; i++) {
            if (contains(node->chd[i], temp)) {
                insertBody(opts, node->chd[i], temp); // 遞迴插入
                break;
            }
        }

        // 創建虛擬粒子，代表節點的質心
        node->data = (particle*)malloc(sizeof(particle));
        memset(node->data, 0, sizeof(particle));
        node->data->x = temp->x; 
        node->data->y = temp->y;
        node->data->mass = temp->mass; 
        node->data->index = -1;
        node->bIdx = -1;
        node->has_children = true;
    }

    // 將當前粒子插入到正確的子節點
    for (int i = 0; i < 4; i++) {
        if (contains(node->chd[i], p)) {
            insertBody(opts, node->chd[i], p); // 遞迴插入
            break;
        }
    }

    // 更新節點的質心和總質量
    double totalMass = node->data->mass + p->mass;
    node->data->x = (node->data->mass * node->data->x + p->mass * p->x) / totalMass;
    node->data->y = (node->data->mass * node->data->y + p->mass * p->y) / totalMass;
    node->data->mass = totalMass;
}

void splitNode(Node* node) {
    double xmin = node->min_bound[0], ymin = node->min_bound[1];
    double xmax = node->max_bound[0], ymax = node->max_bound[1];
    double midX = xmin + (xmax - xmin) / 2;
    double midY = ymin + (ymax - ymin) / 2;

    for (int i = 0; i < 4; i++) {
        double minB[2], maxB[2];
        switch (i) {
            case 0: minB[0] = midX; minB[1] = midY; maxB[0] = xmax; maxB[1] = ymax; break; // 東北
            case 1: minB[0] = xmin; minB[1] = midY; maxB[0] = midX; maxB[1] = ymax; break; // 西北
            case 2: minB[0] = midX; minB[1] = ymin; maxB[0] = xmax; maxB[1] = midY; break; // 東南
            case 3: minB[0] = xmin; minB[1] = ymin; maxB[0] = midX; maxB[1] = midY; break; // 西南
        }

        node->chd[i] = (Node*)malloc(sizeof(Node));
        memset(node->chd[i], 0, sizeof(Node));
        node->chd[i]->min_bound[0] = minB[0];
        node->chd[i]->min_bound[1] = minB[1];
        node->chd[i]->max_bound[0] = maxB[0];
        node->chd[i]->max_bound[1] = maxB[1];
        node->chd[i]->has_children = false;
        node->chd[i]->s = node->s / 2;
        node->chd[i]->parent = node;
    }
}

#include <stdio.h>

void printTree(struct Node *node) {
    if (node) {
        // 打印當前節點的信息
        printf("===================================================\n");
        printf("s=%lf min=(%le,%le) max=(%le,%le) div=%d\n",
               node->s, node->min_bound[0], node->min_bound[1],
               node->max_bound[0], node->max_bound[1], node->has_children);
        if (node->data) {
            printf("mass=%le (x,y)=(%le,%le) Idx=%d\n",
                   node->data->mass, node->data->x,
                   node->data->y, node->bIdx);
        }
        printf("===================================================\n");

        // 遞歸打印所有子節點
        for (int i = 0; i < 4; i++) {
            printTree(node->chd[i]);
        }
    }
}