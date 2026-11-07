#pragma once

#include "esp_err.h"

/* 初始化所有硬件并创建应用状态。 */
esp_err_t robot_app_init(void);

/* 启动正式机器人循环，本函数不会返回。 */
void robot_app_run(void);
