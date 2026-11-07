#include "servo.h"

#include "driver/ledc.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "robot_config.h"
#include "servo_control.h"

static const char *TAG = "robot_servo";
static servo_motion_t s_pan;
static servo_motion_t s_tilt;
static bool s_initialized;
static uint32_t s_last_pan_target_us;
static uint32_t s_last_tilt_target_us;

static const ledc_mode_t k_speed_mode = LEDC_LOW_SPEED_MODE;
static const ledc_timer_t k_timer = LEDC_TIMER_0;
static const ledc_channel_t k_pan_channel = LEDC_CHANNEL_0;
static const ledc_channel_t k_tilt_channel = LEDC_CHANNEL_1;

/* 占空比换算必须和 LEDC 定时器的实际分辨率保持一致。 */
_Static_assert(ROBOT_SERVO_PWM_RESOLUTION_BITS == 14,
               "servo duty conversion must match LEDC_TIMER_14_BIT");

static esp_err_t configure_channel(int gpio_num, ledc_channel_t channel, uint32_t initial_us)
{
    /* 初始化时就输出默认脉宽，避免舵机上电后没有保持信号。 */
    const ledc_channel_config_t config = {
        .gpio_num = gpio_num,
        .speed_mode = k_speed_mode,
        .channel = channel,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = k_timer,
        .duty = servo_pulse_us_to_duty(initial_us,
                                       ROBOT_SERVO_PWM_HZ,
                                       ROBOT_SERVO_PWM_RESOLUTION_BITS),
        .hpoint = 0,
    };
    return ledc_channel_config(&config);
}

static esp_err_t set_pulse_width(ledc_channel_t channel, uint32_t pulse_us)
{
    /* 先写比较值，再调用 update 让新占空比在下一个 PWM 周期生效。 */
    const uint32_t duty = servo_pulse_us_to_duty(pulse_us,
                                                  ROBOT_SERVO_PWM_HZ,
                                                  ROBOT_SERVO_PWM_RESOLUTION_BITS);
    esp_err_t err = ledc_set_duty(k_speed_mode, channel, duty);
    if (err != ESP_OK) {
        return err;
    }
    return ledc_update_duty(k_speed_mode, channel);
}

static void disable_outputs(void)
{
    /* 初始化失败时停掉两路 PWM，避免输出不确定的脉冲。 */
    ledc_stop(k_speed_mode, k_pan_channel, 0);
    ledc_stop(k_speed_mode, k_tilt_channel, 0);
}

esp_err_t robot_servo_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }

    /* 两路 SG90 共用同一个 50Hz 定时器，只使用不同 LEDC 通道。 */
    const ledc_timer_config_t timer_config = {
        .speed_mode = k_speed_mode,
        .duty_resolution = LEDC_TIMER_14_BIT,
        .timer_num = k_timer,
        .freq_hz = ROBOT_SERVO_PWM_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };

    esp_err_t err = ledc_timer_config(&timer_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "LEDC timer initialization failed: %s", esp_err_to_name(err));
        return err;
    }

    /* 平滑状态以微秒计时，步进单位与舵机脉宽单位一致。 */
    const uint64_t now_us = esp_timer_get_time();
    servo_motion_init(&s_pan, ROBOT_SERVO_PAN_MIN_US, ROBOT_SERVO_PAN_MAX_US,
                      ROBOT_SERVO_PAN_HOME_US, ROBOT_SERVO_STEP_US,
                      ROBOT_SERVO_RAMP_STEP_US, now_us);
    servo_motion_init(&s_tilt, ROBOT_SERVO_TILT_MIN_US, ROBOT_SERVO_TILT_MAX_US,
                      ROBOT_SERVO_TILT_HOME_US, ROBOT_SERVO_STEP_US,
                      ROBOT_SERVO_RAMP_STEP_US, now_us);

    err = configure_channel(SERVO_PAN_GPIO, k_pan_channel, s_pan.current_us);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Pan servo initialization failed: %s", esp_err_to_name(err));
        disable_outputs();
        return err;
    }

    err = configure_channel(SERVO_TILT_GPIO, k_tilt_channel, s_tilt.current_us);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Tilt servo initialization failed: %s", esp_err_to_name(err));
        disable_outputs();
        return err;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "SG90 servos initialized at pan=%lu us, tilt=%lu us",
             (unsigned long)s_pan.current_us, (unsigned long)s_tilt.current_us);
    return ESP_OK;
}

void robot_servo_set_targets(const robot_action_t *action)
{
    if (action) {
        if (action->pan_us != s_last_pan_target_us ||
            action->tilt_us != s_last_tilt_target_us) {
            ESP_LOGI(TAG, "servo target: pan=%lu us, tilt=%lu us",
                     (unsigned long)action->pan_us,
                     (unsigned long)action->tilt_us);
            s_last_pan_target_us = action->pan_us;
            s_last_tilt_target_us = action->tilt_us;
        }
        servo_motion_set_target(&s_pan, action->pan_us);
        servo_motion_set_target(&s_tilt, action->tilt_us);
    }
}

void robot_servo_update(void)
{
    if (!s_initialized) {
        return;
    }

    /* 正常控制循环中每次只推进一小步，不阻塞主循环。 */
    const uint64_t now_us = esp_timer_get_time();
    uint32_t next_us = 0;

    if (servo_motion_update(&s_pan, now_us, ROBOT_SERVO_UPDATE_INTERVAL_US, &next_us)) {
        esp_err_t err = set_pulse_width(k_pan_channel, next_us);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Pan PWM update failed: %s", esp_err_to_name(err));
        }
    }

    if (servo_motion_update(&s_tilt, now_us, ROBOT_SERVO_UPDATE_INTERVAL_US, &next_us)) {
        esp_err_t err = set_pulse_width(k_tilt_channel, next_us);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Tilt PWM update failed: %s", esp_err_to_name(err));
        }
    }
}
