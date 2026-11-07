#include "board_led.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "robot_config.h"

static const char *TAG = "board_led";
static bool s_initialized;

esp_err_t board_led_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }

    /* 标准版板载蓝灯低电平点亮，初始化后先保持熄灭。 */
    const gpio_config_t config = {
        .pin_bit_mask = (1ULL << BOARD_LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t err = gpio_config(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "GPIO initialization failed: %s", esp_err_to_name(err));
        return err;
    }
    s_initialized = true;
    return board_led_set(false);
}

esp_err_t board_led_set(bool on)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    /* 配置值 0 表示低电平点亮；熄灭时写入相反电平。 */
    const uint32_t level = on ? BOARD_LED_ACTIVE_LEVEL : !BOARD_LED_ACTIVE_LEVEL;
    return gpio_set_level(BOARD_LED_GPIO, level);
}

