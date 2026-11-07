#pragma once

#include "esp_err.h"
#include "robot_types.h"

/* 配置红外和触摸 GPIO。 */
esp_err_t robot_input_init(void);

/* 采样并防抖，结果写入 inputs。 */
void robot_input_update(robot_inputs_t *inputs, uint32_t now_ms);
