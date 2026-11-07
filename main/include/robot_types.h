#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    ROBOT_EXPRESSION_NORMAL = 0,  /* 普通睁眼表情 */
    ROBOT_EXPRESSION_CLOSED,      /* 眨眼或闭眼表情 */
    ROBOT_EXPRESSION_NEAR,        /* 检测到靠近物体时的表情 */
    ROBOT_EXPRESSION_TOUCHED,     /* 触摸头部时的表情 */
} robot_expression_t;

/* 一次输入采样结果，true 表示对应功能已触发。 */
typedef struct {
    bool touch;
    bool ir_left;
    bool ir_right;
} robot_inputs_t;

/* 行为模块计算出的目标姿势和表情。pan/tilt 单位均为微秒。 */
typedef struct {
    uint32_t pan_us;
    uint32_t tilt_us;
    robot_expression_t expression;
} robot_action_t;
