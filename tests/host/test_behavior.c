#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "behavior.h"
#include "robot_config.h"

static robot_inputs_t inputs(bool touch, bool left, bool right)
{
    return (robot_inputs_t){
        .touch = touch,
        .ir_left = left,
        .ir_right = right,
    };
}

static void test_ir_direction_mapping(void)
{
    robot_behavior_state_t state;
    robot_action_t action;
    robot_behavior_init(&state, 0);

    /* 左侧单独触发应向左看。 */
    const robot_inputs_t left = inputs(false, true, false);
    robot_behavior_update(&state, &left, 100, &action);
    assert(action.pan_us == ROBOT_SERVO_PAN_LEFT_US);
    assert(action.expression == ROBOT_EXPRESSION_NEAR);

    /* 右侧单独触发应向右看。 */
    const robot_inputs_t right = inputs(false, false, true);
    robot_behavior_update(&state, &right, 200, &action);
    assert(action.pan_us == ROBOT_SERVO_PAN_RIGHT_US);

    /* 两侧同时触发时无法判断方向，应回到默认朝向。 */
    const robot_inputs_t both = inputs(false, true, true);
    robot_behavior_update(&state, &both, 300, &action);
    assert(action.pan_us == ROBOT_SERVO_PAN_HOME_US);
    assert(action.expression == ROBOT_EXPRESSION_NEAR);
}

static void test_touch_priority_and_hold(void)
{
    robot_behavior_state_t state;
    robot_action_t action;
    robot_behavior_init(&state, 0);

    /* 触摸优先于红外，并执行低头。 */
    const robot_inputs_t touch_with_ir = inputs(true, true, false);
    robot_behavior_update(&state, &touch_with_ir, 100, &action);
    assert(action.pan_us == ROBOT_SERVO_PAN_HOME_US);
    assert(action.tilt_us == ROBOT_SERVO_TILT_DOWN_US);
    assert(action.expression == ROBOT_EXPRESSION_TOUCHED);

    /* 松手后 1 秒内仍保持触摸反馈。 */
    const robot_inputs_t released = inputs(false, false, false);
    robot_behavior_update(&state, &released, 200, &action);
    assert(action.expression == ROBOT_EXPRESSION_TOUCHED);

    robot_behavior_update(&state, &released, 1099, &action);
    assert(action.expression == ROBOT_EXPRESSION_TOUCHED);

    robot_behavior_update(&state, &released, 1100, &action);
    assert(action.expression == ROBOT_EXPRESSION_NORMAL);
}

static void test_ir_return_delay(void)
{
    robot_behavior_state_t state;
    robot_action_t action;
    robot_behavior_init(&state, 0);

    const robot_inputs_t left = inputs(false, true, false);
    const robot_inputs_t none = inputs(false, false, false);
    robot_behavior_update(&state, &left, 100, &action);

    /* 手离开后先保持原方向约 400ms。 */
    robot_behavior_update(&state, &none, 200, &action);
    assert(action.pan_us == ROBOT_SERVO_PAN_LEFT_US);
    robot_behavior_update(&state, &none, 499, &action);
    assert(action.pan_us == ROBOT_SERVO_PAN_LEFT_US);

    /* 超过保持时间后回到默认位置。 */
    robot_behavior_update(&state, &none, 500, &action);
    assert(action.pan_us == ROBOT_SERVO_PAN_HOME_US);
    assert(action.expression == ROBOT_EXPRESSION_NORMAL);
}

static void test_normal_face_blinks(void)
{
    robot_behavior_state_t state;
    robot_action_t action;
    robot_behavior_init(&state, 0);
    const robot_inputs_t none = inputs(false, false, false);

    robot_behavior_update(&state, &none, 0, &action);
    assert(action.expression == ROBOT_EXPRESSION_NORMAL);

    /* 普通表情每 3 秒眨眼 180ms。 */
    robot_behavior_update(&state, &none, 2999, &action);
    assert(action.expression == ROBOT_EXPRESSION_NORMAL);
    robot_behavior_update(&state, &none, 3000, &action);
    assert(action.expression == ROBOT_EXPRESSION_CLOSED);
    robot_behavior_update(&state, &none, 3179, &action);
    assert(action.expression == ROBOT_EXPRESSION_CLOSED);
    robot_behavior_update(&state, &none, 3180, &action);
    assert(action.expression == ROBOT_EXPRESSION_NORMAL);
}

int main(void)
{
    test_ir_direction_mapping();
    test_touch_priority_and_hold();
    test_ir_return_delay();
    test_normal_face_blinks();
    puts("behavior tests passed");
    return 0;
}
