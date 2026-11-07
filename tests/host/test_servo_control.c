#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "servo_control.h"

static void test_pulse_to_duty(void)
{
    assert(servo_pulse_us_to_duty(500, 50, 14) == 409);
    assert(servo_pulse_us_to_duty(1500, 50, 14) == 1228);
    assert(servo_pulse_us_to_duty(2500, 50, 14) == 2048);
    assert(servo_pulse_us_to_duty(1500, 0, 14) == 0);
}

static void test_target_clamping(void)
{
    servo_motion_t motion;
    servo_motion_init(&motion, 1200, 1800, 1500, 4000, 1000, 0);

    servo_motion_set_target(&motion, 2500);
    assert(motion.target_us == 1800);
    servo_motion_set_target(&motion, 900);
    assert(motion.target_us == 1200);
}

static void test_smooth_motion_reaches_target(void)
{
    servo_motion_t motion;
    uint32_t output_us = 0;
    servo_motion_init(&motion, 1200, 1800, 1500, 4000, 1000, 0);
    servo_motion_set_target(&motion, 1800);

    assert(servo_motion_update(&motion, 0, 20000, &output_us));
    assert(output_us == 1600);
    assert(!servo_motion_update(&motion, 19999, 20000, &output_us));
    assert(servo_motion_update(&motion, 20000, 20000, &output_us));
    assert(output_us == 1700);
    assert(servo_motion_update(&motion, 40000, 20000, &output_us));
    assert(output_us == 1800);
    assert(!servo_motion_update(&motion, 60000, 20000, &output_us));
}

static void test_direction_change_restarts_ramp(void)
{
    servo_motion_t motion;
    uint32_t output_us = 0;
    servo_motion_init(&motion, 1200, 1800, 1200, 1000, 100, 0);
    servo_motion_set_target(&motion, 1800);

    assert(servo_motion_update(&motion, 0, 20000, &output_us));
    assert(output_us == 1300);
    assert(servo_motion_update(&motion, 20000, 20000, &output_us));
    assert(output_us == 1425);

    servo_motion_set_target(&motion, 1200);
    assert(servo_motion_update(&motion, 40000, 20000, &output_us));
    assert(output_us == 1325);
}

int main(void)
{
    test_pulse_to_duty();
    test_target_clamping();
    test_smooth_motion_reaches_target();
    test_direction_change_restarts_ramp();
    puts("servo control tests passed");
    return 0;
}
