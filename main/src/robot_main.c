#include "esp_err.h"
#include "esp_log.h"
#include "robot_app.h"

static const char *TAG = "cardboard_robot";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting cardboard robot");

    /* 正式固件只保留应用启动入口，所有硬件编排集中在 robot_app。 */
    const esp_err_t err = robot_app_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Application initialization failed: %s", esp_err_to_name(err));
        return;
    }
    robot_app_run();
}
