#pragma once

struct particle {
    int index;
    double x, y;
    double mass;
    double v_x, v_y;
    double a_x = 0.0, a_y = 0.0;  // 預設加速度為 0
    //double pos[2];
    //double vel[2];
};
