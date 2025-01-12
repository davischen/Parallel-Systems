#include <vector>
#include <utility> // for std::pair
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <cmath> // 用于数学计算
#include "particle.h"
#include "tree.h"
#include <thread>


// 粒子轨迹
std::vector<std::vector<std::pair<float, float>>> trails;

// OpenGL 窗口指针
GLFWwindow* window;

// OpenGL 窗口尺寸
const int WIN_WIDTH = 1000, WIN_HEIGHT = 1000;

// OpenGL 显示比例
float MAX_X = 1.0f, MAX_Y = 1.0f;

// 坐标转换
#define WIN_X(x) ((2.0f * (x) / MAX_X) - 1.0f)
#define WIN_Y(y) ((2.0f * (y) / MAX_Y) - 1.0f)

// 键盘事件处理
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

// 初始化 OpenGL 可视化
int init_visualization() {
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

    window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "Simulation", NULL, NULL);
    if (window == NULL) {
        fprintf(stderr, "Failed to open GLFW window.\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    
    return 0;
}

// 绘制四分树边界
void drawOctreeBounds2D(const Node* node) {
    if (node == nullptr) return;

    glBegin(GL_LINES);
    glColor3f(0.8f, 0.8f, 0.8f); // 灰色线条

    // 垂直线
    glVertex2f(WIN_X(node->min_bound[0]), WIN_Y(node->min_bound[1]));
    glVertex2f(WIN_X(node->min_bound[0]), WIN_Y(node->max_bound[1]));

    glVertex2f(WIN_X(node->max_bound[0]), WIN_Y(node->min_bound[1]));
    glVertex2f(WIN_X(node->max_bound[0]), WIN_Y(node->max_bound[1]));

    // 水平线
    glVertex2f(WIN_X(node->min_bound[0]), WIN_Y(node->min_bound[1]));
    glVertex2f(WIN_X(node->max_bound[0]), WIN_Y(node->min_bound[1]));

    glVertex2f(WIN_X(node->min_bound[0]), WIN_Y(node->max_bound[1]));
    glVertex2f(WIN_X(node->max_bound[0]), WIN_Y(node->max_bound[1]));

    glEnd();

    if (node->has_children) {
        for (int i = 0; i < 4; ++i) {
            drawOctreeBounds2D(node->chd[i]);
        }
    }
}
/*void renderText(const char* text, float x, float y) {
    glRasterPos2f(x, y); // 設置文字位置
    for (const char* c = text; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *c); // 或者替換字體
    }
}*/


// 可视化渲染函数
bool render_visualization(int n_particles, const particle* particles, const Node* root, int step) {
    glClear(GL_COLOR_BUFFER_BIT);

    // 动态计算 MAX_X 和 MAX_Y
    for (int i = 0; i < n_particles; i++) {
        if (particles[i].x > MAX_X) MAX_X = particles[i].x;
        if (particles[i].y > MAX_Y) MAX_Y = particles[i].y;
    }

    // 绘制粒子的轨迹和位置
    if (trails.empty()) {
        trails.resize(n_particles); // 初始化轨迹
    }

    for (int i = 0; i < n_particles; i++) {
        auto win_x = WIN_X(particles[i].x);
        auto win_y = WIN_Y(particles[i].y);

        trails[i].emplace_back(win_x, win_y);
        if (trails[i].size() > 50) {
            trails[i].erase(trails[i].begin());
        }

        // 绘制轨迹
        glBegin(GL_LINE_STRIP);
        glColor3f(0.6f, 0.6f, 0.6f); // 灰色
        for (const auto& point : trails[i]) {
            glVertex2f(point.first, point.second);
        }
        glEnd();

        // 绘制粒子
        glBegin(GL_TRIANGLE_FAN);
        glColor3f(0.1f, 0.3f, 0.6f); // 蓝色
        glVertex2f(win_x, win_y);
        float radius = 0.005f; // 粒子大小
        for (int j = 0; j <= 20; ++j) {
            float angle = j * 2.0f * M_PI / 20;
            glVertex2f(win_x + radius * cos(angle), win_y + radius * sin(angle));
        }
        glEnd();
    }

    // 绘制四分树边界
    drawOctreeBounds2D(root);

    // 繪製文字，顯示 step
    //char stepText[50];
    //snprintf(stepText, sizeof(stepText), "Step: %d", step);
    //renderText(stepText, -0.95f, 0.9f); // 左上角顯示
    //printf("%d",step);

    glfwSwapBuffers(window);
    glfwPollEvents();
    if(step == 0){
        printf("This is first draw. Press SPACE to continue...\n");
        while (true) {
            glfwPollEvents();
            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
                printf("Continue...\n");
                std::this_thread::sleep_for(std::chrono::milliseconds(200)); // 防止過度觸發
                break; // 繼續執行
            }
        }
    }
    return glfwWindowShouldClose(window); // 返回窗口是否應該關閉
}


// 终止 OpenGL
void terminate_visualization() {
    printf("Press Q to quit...\n");
    while (true) {
        //
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
            break; // 退出
        }
    }
    glfwDestroyWindow(window);
    glfwTerminate(); // 結束 OpenGL
}