#pragma once

#include "esp_err.h"
#include "robot_types.h"

/* 初始化 LEDC 定时器和两路 SG90 PWM。 */
esp_err_t robot_servo_init(void);

/* 更新目标位置；实际转动由 robot_servo_update 平滑推进。 */
void robot_servo_set_targets(const robot_action_t *action);

/* 按控制周期调用，每次向目标脉宽靠近一小步。 */
void robot_servo_update(void);
