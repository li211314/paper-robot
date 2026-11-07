#pragma once

#include <stdbool.h>
#include <stdint.h>

/*
 * 一路位置的平滑运动状态。
 * 所有脉宽字段单位都是微秒，时间字段单位都是微秒。
 */
typedef struct {
    uint32_t min_us;          /* 软件允许的最小脉宽 */
    uint32_t max_us;          /* 软件允许的最大脉宽 */
    uint32_t current_us;      /* 当前已输出的脉宽 */
    uint32_t target_us;       /* 行为模块要求到达的脉宽 */
    uint32_t step_limit_us;   /* 单次更新允许的最大变化量 */
    uint32_t ramp_step_us;    /* 起步和靠近目标时的缓动增量 */
    uint32_t step_now_us;     /* 本轮实际使用的缓动步长 */
    int8_t direction;         /* 1=增加脉宽，-1=减小脉宽，0=停止 */
    uint64_t next_update_us;  /* 下一次允许更新的时间 */
} servo_motion_t;

/* 把 SG90 脉宽换算成 LEDC 占空比计数值。 */
uint32_t servo_pulse_us_to_duty(uint32_t pulse_us,
                                uint32_t frequency_hz,
                                uint8_t resolution_bits);

/* 初始化一路平滑运动状态。 */
void servo_motion_init(servo_motion_t *motion,
                       uint32_t min_us,
                       uint32_t max_us,
                       uint32_t initial_us,
                       uint32_t step_limit_us,
                       uint32_t ramp_step_us,
                       uint64_t now_us);

/* 设置目标脉宽，超出安全范围时自动限幅。 */
void servo_motion_set_target(servo_motion_t *motion, uint32_t target_us);

/*
 * 到达更新时间时向目标推进一步。
 * 返回 true 表示 current_us 已变化，需要更新 PWM。
 */
bool servo_motion_update(servo_motion_t *motion,
                         uint64_t now_us,
                         uint32_t interval_us,
                         uint32_t *current_us);
