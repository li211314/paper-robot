#include "behavior.h"

#include "robot_config.h"

static void set_action(robot_action_t *action,
                       uint32_t pan_us,
                       uint32_t tilt_us,
                       robot_expression_t expression)
{
    /* 行为层只给出目标和表情，不直接操作 GPIO 或 PWM。 */
    action->pan_us = pan_us;
    action->tilt_us = tilt_us;
    action->expression = expression;
}

static bool time_reached(uint32_t now_ms, uint32_t deadline_ms)
{
    /* 使用有符号差值，正确处理 32 位毫秒计数回绕。 */
    return (int32_t)(now_ms - deadline_ms) >= 0;
}

void robot_behavior_init(robot_behavior_state_t *state, uint32_t now_ms)
{
    if (state) {
        *state = (robot_behavior_state_t){0};
        state->last_blink_ms = now_ms;
    }
}

void robot_behavior_update(robot_behavior_state_t *state,
                           const robot_inputs_t *inputs,
                           uint32_t now_ms,
                           robot_action_t *action)
{
    if (!state || !inputs || !action) {
        return;
    }

    if (inputs->touch) {
        state->touch_latched = true;
        state->touch_hold_until_ms = now_ms + ROBOT_TOUCH_HOLD_MS;
    } else if (state->touch_latched &&
               time_reached(now_ms, state->touch_hold_until_ms)) {
        state->touch_latched = false;
    }

    uint32_t pan_us = ROBOT_SERVO_PAN_HOME_US;
    uint32_t tilt_us = ROBOT_SERVO_TILT_HOME_US;
    robot_expression_t expression = ROBOT_EXPRESSION_NORMAL;

    if (state->touch_latched) {
        /* 触摸优先级最高：不管红外看到什么，都回中并低头。 */
        tilt_us = ROBOT_SERVO_TILT_DOWN_US;
        expression = ROBOT_EXPRESSION_TOUCHED;
    } else if (inputs->ir_left || inputs->ir_right) {
        state->has_ir_history = true;
        state->last_ir_seen_ms = now_ms;
        tilt_us = ROBOT_SERVO_TILT_NEAR_US;
        expression = ROBOT_EXPRESSION_NEAR;

        if (inputs->ir_left && !inputs->ir_right) {
            pan_us = ROBOT_SERVO_PAN_LEFT_US;
        } else if (inputs->ir_right && !inputs->ir_left) {
            pan_us = ROBOT_SERVO_PAN_RIGHT_US;
        } else {
            /* 两侧同时触发时无法判断方向，保持默认朝向。 */
            pan_us = ROBOT_SERVO_PAN_HOME_US;
        }
        state->last_ir_pan_us = pan_us;
    } else if (state->has_ir_history &&
               !time_reached(now_ms, state->last_ir_seen_ms + ROBOT_RETURN_DELAY_MS)) {
        /* 手短暂离开检测区时保留当前方向，避免头部来回抖动。 */
        pan_us = state->last_ir_pan_us;
        tilt_us = ROBOT_SERVO_TILT_NEAR_US;
        expression = ROBOT_EXPRESSION_NEAR;
    } else {
        state->has_ir_history = false;
    }

    if (expression == ROBOT_EXPRESSION_NORMAL) {
        if (time_reached(now_ms, state->last_blink_ms + ROBOT_BLINK_INTERVAL_MS)) {
            state->blink_until_ms = now_ms + ROBOT_BLINK_DURATION_MS;
            state->last_blink_ms = now_ms;
        }
        if (!time_reached(now_ms, state->blink_until_ms)) {
            expression = ROBOT_EXPRESSION_CLOSED;
        }
    } else {
        /* 有其他表情时不眨眼，并重新计算下一次普通表情的眨眼时间。 */
        state->blink_until_ms = now_ms;
        state->last_blink_ms = now_ms;
    }

    set_action(action, pan_us, tilt_us, expression);
}
