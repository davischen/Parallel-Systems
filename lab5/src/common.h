#ifndef COMMON_H
#define COMMON_H

// 條件編譯，控制性能輸出
#ifdef TIMING
#define TIMING_TEST 1
#else
#define TIMING_TEST 0
#endif

#ifdef DEBUG
#define DEBUG_TEST 1
#else
#define DEBUG_TEST 0
#endif

#define TIMING_PRINT(x) do { if (TIMING_TEST) { printf("TIMING: "); x;} } while (0)
#define DEBUG_PRINT(x) do { if (DEBUG_TEST) { printf("DEBUG: "); x;} } while (0)

constexpr double OUT_OF_BOUNDS_MASS = -1.0; // 表示粒子已超出邊界
constexpr double MIN_X = 0.0;               // 領域最小 X 值
constexpr double MAX_X = 4.0;               // 領域最大 X 值
constexpr double MIN_Y = 0.0;               // 領域最小 Y 值
constexpr double MAX_Y = 4.0;               // 領域最大 Y 值
constexpr double G = 0.0001;                // 引力常數
constexpr double RLIMIT = 0.03;             // 最小距離限制，避免除以零

const static double MS_PER_S = 1000.0f; // 毫秒與秒之間的轉換
const static double NS_PER_MS = 1000.0f; // 納秒與毫秒之間的轉換


#endif // COMMON_H