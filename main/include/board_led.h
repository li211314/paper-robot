#pragma once

#include <stdbool.h>

#include "esp_err.h"

/* 标准版 Super Mini 的蓝色用户灯，GPIO8，低电平点亮。 */
esp_err_t board_led_init(void);

/* on=true 点亮，on=false 熄灭。 */
esp_err_t board_led_set(bool on);
