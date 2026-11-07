#include "servo_control.h"

static uint32_t clamp_u32(uint32_t value, uint32_t minimum, uint32_t maximum)
{
    /* 所有外部目标都经过这里，防止误配置把舵机顶到机构端点。 */
    if (value < minimum) {
        return minimum;
    }
    if (value > maximum) {
        return maximum;
    }
    return value;
}

uint32_t servo_pulse_us_to_duty(uint32_t pulse_us,
                                uint32_t frequency_hz,
                                uint8_t resolution_bits)
{
    if (frequency_hz == 0 || resolution_bits == 0 || resolution_bits > 31) {
        return 0;
    }

    /*
     * 占空比 = 目标脉宽 / PWM周期 * (2^分辨率 - 1)。
     * 先用 64 位运算，避免中间乘法溢出。
     */
    const uint64_t scale = UINT64_C(1) << resolution_bits;
    return (uint32_t)(((uint64_t)pulse_us * frequency_hz * scale) / 1000000ULL);
}

void servo_motion_init(servo_motion_t *motion,
                       uint32_t min_us,
                       uint32_t max_us,
                       uint32_t initial_us,
                       uint32_t step_limit_us,
                       uint32_t ramp_step_us,
                       uint64_t now_us)
{
    if (!motion) {
        return;
    }

    motion->min_us = min_us;
    motion->max_us = max_us;
    motion->current_us = clamp_u32(initial_us, min_us, max_us);
    motion->target_us = motion->current_us;
    motion->step_limit_us = step_limit_us;
    motion->ramp_step_us = ramp_step_us;
    motion->step_now_us = 0;
    motion->direction = 0;
    motion->next_update_us = now_us;
}

void servo_motion_set_target(servo_motion_t *motion, uint32_t target_us)
{
    if (motion) {
        motion->target_us = clamp_u32(target_us, motion->min_us, motion->max_us);
    }
}

bool servo_motion_update(servo_motion_t *motion,
                         uint64_t now_us,
                         uint32_t interval_us,
                         uint32_t *current_us)
{
    if (!motion || now_us < motion->next_update_us) {
        return false;
    }

    motion->next_update_us = now_us + interval_us;

    /* 使用有符号差值，目标小于当前位置时会得到负方向。 */
    const int32_t remaining = (int32_t)motion->target_us - (int32_t)motion->current_us;
    if (remaining == 0) {
        motion->step_now_us = 0;
        motion->direction = 0;
        return false;
    }

    const int8_t direction = (remaining > 0) ? 1 : -1;
    /* 中途反向时清空已积累的步长，从缓动起步重新加速。 */
    if (direction != motion->direction) {
        motion->step_now_us = 0;
    }
    motion->direction = direction;

    const uint32_t magnitude = (uint32_t)((remaining > 0) ? remaining : -remaining);
    /*
     * 距离越远步长越大，距离越近步长越小；再受 step_limit_us
     * 和 ramp_step_us 双重限制，形成柔和的起停效果。
     */
    uint32_t desired_step = magnitude / 4U;
    if (desired_step < motion->ramp_step_us) {
        desired_step = motion->ramp_step_us;
    }
    if (desired_step > motion->step_limit_us) {
        desired_step = motion->step_limit_us;
    }

    /* 每轮只增加 ramp_step_us，不要一步跨到最大速度。 */
    motion->step_now_us += motion->ramp_step_us;
    if (motion->step_now_us > desired_step) {
        motion->step_now_us = desired_step;
    }

    uint32_t delta = motion->step_now_us;
    if (delta > magnitude) {
        delta = magnitude;
    }

    if (direction > 0) {
        motion->current_us += delta;
    } else {
        motion->current_us -= delta;
    }

    if (current_us) {
        *current_us = motion->current_us;
    }
    return true;
}
