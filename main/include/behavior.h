#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "robot_types.h"

typedef struct {
    bool touch_latched;
    uint32_t touch_hold_until_ms;
    bool has_ir_history;
    uint32_t last_ir_seen_ms;
    uint32_t last_ir_pan_us;
    uint32_t last_blink_ms;
    uint32_t blink_until_ms;
} robot_behavior_state_t;

/* 初始化状态机中的计时基准。 */
void robot_behavior_init(robot_behavior_state_t *state, uint32_t now_ms);

/*
 * 根据已防抖输入和当前时间生成目标动作。
 * 优先级：触摸 > 左/右红外 > 空闲回中。
 */
void robot_behavior_update(robot_behavior_state_t *state,
                           const robot_inputs_t *inputs,
                           uint32_t now_ms,
                           robot_action_t *action);
