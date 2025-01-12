#ifndef TREE_H
#define TREE_H

#include <array>
#include <vector>
#include "argparse.h"
#include "particle.h"

struct Node {
    double min_bound[2], max_bound[2];
    double s;           // 節點的大小
    bool has_children;
    particle* data;        // 指向包含的粒子
    Node* chd[4];       // 四個子節點
    Node* parent;       // 父節點
    int bIdx;           // 粒子的索引
};


void insertBody(const options_t* opts, Node* node, particle* body);
void compute_force(const options_t* opts, const Node* node, particle* p);
//std::array<double, 2> compute_force_correct(const options_t* opts, const Node* node, particle* p);
bool contains(const Node* node, const particle* body);
void tearDownTree(Node* node);
void initialize_root(Node* root);
//void insertBody_v2(const options_t* opts, Node* node, particle* p);
void updateParticleState_v2(particle* b, const std::array<double, 2>& forces, double timestep, const Node* root);
void updateParticleState(particle* b, double timestep, const Node* root);
void splitNode(Node* node);
std::array<double, 2> compute_force_v2(const options_t* opts, const Node* node, particle* b);
void printTree(struct Node *node);
#endif
