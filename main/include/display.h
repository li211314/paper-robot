#pragma once

#include "esp_err.h"
#include "robot_types.h"

/* 初始化 I2C 总线和 SSD1306 屏幕。 */
esp_err_t robot_display_init(void);

/* 绘制指定表情并刷新到屏幕。 */
void robot_display_render(robot_expression_t expression);
