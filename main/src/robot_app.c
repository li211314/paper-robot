#include "robot_app.h"

#include "behavior.h"
#include "board_led.h"
#include "display.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "input.h"
#include "robot_config.h"
#include "servo.h"

static const char *TAG = "robot_app";

static robot_inputs_t s_inputs;
static robot_behavior_state_t s_behavior;
static bool s_display_ready;
static robot_expression_t s_displayed_expression;
static bool s_first_render;
static robot_action_t s_last_logged_action;
static bool s_has_last_logged_action;

static const char *expression_name(robot_expression_t expression)
{
    switch (expression) {
    case ROBOT_EXPRESSION_CLOSED:
        return "closed";
    case ROBOT_EXPRESSION_NEAR:
        return "near";
    case ROBOT_EXPRESSION_TOUCHED:
        return "touched";
    case ROBOT_EXPRESSION_NORMAL:
    default:
        return "normal";
    }
}

static uint32_t app_now_ms(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

static void log_optional_error(const char *module, esp_err_t err)
{
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "%s initialization failed: %s", module, esp_err_to_name(err));
    }
}

esp_err_t robot_app_init(void)
{
    /* 输入和舵机属于闭环控制必需项，初始化失败时终止启动。 */
    esp_err_t err = robot_input_init();
    if (err != ESP_OK) {
        return err;
    }

    err = robot_servo_init();
    if (err != ESP_OK) {
        return err;
    }

    /* 屏幕和状态灯不是闭环控制必需项，失败时只记录日志。 */
    const esp_err_t display_err = robot_display_init();
    log_optional_error("display", display_err);
    s_display_ready = (display_err == ESP_OK);
    log_optional_error("board LED", board_led_init());

    /* 行为状态机需要当前时间作为触摸保持和眨眼的计时基准。 */
    const uint32_t now_ms = app_now_ms();
    robot_behavior_init(&s_behavior, now_ms);

    const robot_action_t initial_action = {
        .pan_us = ROBOT_SERVO_PAN_HOME_US,
        .tilt_us = ROBOT_SERVO_TILT_HOME_US,
        .expression = ROBOT_EXPRESSION_NORMAL,
    };
    robot_servo_set_targets(&initial_action);

    s_last_logged_action = initial_action;
    s_has_last_logged_action = true;
    s_displayed_expression = ROBOT_EXPRESSION_NORMAL;
    s_first_render = true;
    ESP_LOGI(TAG, "Application initialized: pan=%lu tilt=%lu expression=%s",
             (unsigned long)initial_action.pan_us,
             (unsigned long)initial_action.tilt_us,
             expression_name(initial_action.expression));
    return ESP_OK;
}

void robot_app_run(void)
{
    robot_action_t action = {
        .pan_us = ROBOT_SERVO_PAN_HOME_US,
        .tilt_us = ROBOT_SERVO_TILT_HOME_US,
        .expression = ROBOT_EXPRESSION_NORMAL,
    };

    while (true) {
        /* 固定 20ms 周期：先更新输入，再决策，最后输出舵机和表情。 */
        const uint32_t now_ms = app_now_ms();

        robot_input_update(&s_inputs, now_ms);
        robot_behavior_update(&s_behavior, &s_inputs, now_ms, &action);

        /* 只在行为结果变化时打印，便于观察一次触发产生的完整动作。 */
        if (!s_has_last_logged_action ||
            action.pan_us != s_last_logged_action.pan_us ||
            action.tilt_us != s_last_logged_action.tilt_us ||
            action.expression != s_last_logged_action.expression) {
            ESP_LOGI(TAG, "action changed: pan=%lu tilt=%lu expression=%s",
                     (unsigned long)action.pan_us,
                     (unsigned long)action.tilt_us,
                     expression_name(action.expression));
            s_last_logged_action = action;
            s_has_last_logged_action = true;
        }

        robot_servo_set_targets(&action);
        robot_servo_update();

        /* 只有表情变化时才刷新 OLED，避免 I2C 阻塞舵机控制。 */
        if (s_display_ready &&
            (s_first_render || action.expression != s_displayed_expression)) {
            robot_display_render(action.expression);
            s_displayed_expression = action.expression;
            s_first_render = false;
        }

        vTaskDelay(pdMS_TO_TICKS(ROBOT_CONTROL_PERIOD_MS));
    }
}
