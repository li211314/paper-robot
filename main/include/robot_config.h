#pragma once

#include <stdint.h>

/*
 * 项目硬件配置中心。
 * 接线或机械安装变化时，优先修改本文件，不要直接散改各驱动。
 */
enum {
    /* I2C 屏幕：使用 GPIO0/1，避开板载灯 GPIO8 和 BOOT GPIO9。 */
    OLED_SDA_GPIO = 0,
    OLED_SCL_GPIO = 1,

    /* 左右红外按面对屏幕的方向定义，检测到物体时为低电平。 */
    IR_RIGHT_GPIO = 3,
    IR_LEFT_GPIO = 4,

    /* 两路 SG90 的 PWM 信号脚。 */
    SERVO_PAN_GPIO = 5,
    SERVO_TILT_GPIO = 6,

    /* TTP223 触摸输出，触摸时为高电平。 */
    TOUCH_GPIO = 10,

    /* 标准版 Super Mini 的蓝色用户灯。 */
    BOARD_LED_GPIO = 8,
    BOARD_LED_ACTIVE_LEVEL = 0,

    OLED_I2C_ADDRESS = 0x3C,
    OLED_WIDTH = 128,
    OLED_HEIGHT = 64,

    IR_ACTIVE_LEVEL = 0,
    TOUCH_ACTIVE_LEVEL = 1,
};

/* esp-idf-ssd1306 驱动当前固定使用 400kHz。 */
#define ROBOT_OLED_I2C_HZ               400000U

/* SG90：50Hz，周期 20ms；14 位占空比分辨率约等于 1.22us 一步。 */
#define ROBOT_SERVO_PWM_HZ              50U
#define ROBOT_SERVO_PWM_RESOLUTION_BITS 14U

/* 平滑运动参数：单步上限和缓动增量。 */
#define ROBOT_SERVO_STEP_US             4000U
#define ROBOT_SERVO_RAMP_STEP_US        1000U

/* 水平舵机软件限幅，避免纸壳或线材在端点卡住。 */
#define ROBOT_SERVO_PAN_MIN_US          1200U
#define ROBOT_SERVO_PAN_MAX_US          1800U
#define ROBOT_SERVO_PAN_HOME_US         1560U
#define ROBOT_SERVO_PAN_LEFT_US         1200U
#define ROBOT_SERVO_PAN_RIGHT_US        1800U

/* 俯仰舵机的安全范围更窄，首次安装时先保持这个范围。 */
#define ROBOT_SERVO_TILT_MIN_US         1400U
#define ROBOT_SERVO_TILT_MAX_US         1600U
#define ROBOT_SERVO_TILT_HOME_US        1600U
#define ROBOT_SERVO_TILT_NEAR_US        1580U
#define ROBOT_SERVO_TILT_DOWN_US        1400U

/* 主循环和输入/动作时序。 */
#define ROBOT_CONTROL_PERIOD_MS         20U
#define ROBOT_SERVO_UPDATE_INTERVAL_US  (ROBOT_CONTROL_PERIOD_MS * 1000U)
#define ROBOT_INPUT_STABLE_MS           60U
#define ROBOT_RETURN_DELAY_MS           400U
#define ROBOT_TOUCH_HOLD_MS             1000U
#define ROBOT_BLINK_INTERVAL_MS         3000U
#define ROBOT_BLINK_DURATION_MS         180U
