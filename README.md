# Cardboard Robot

基于 ESP32-C3 SuperMini 的小纸壳机器人。

项目使用 ESP-IDF v6.1 和 C 语言开发，计划实现 OLED 表情、两个 SG90 舵机、左右红外感应和触摸互动。

## 当前状态

应用层阶段。

目前已完成输入防抖、触摸保持、红外回中延时、舵机平滑控制和 OLED 表情。
启动不再经过分模块自检，直接进入正式机器人控制流程。

## 运行逻辑

- 上电初始化 GPIO、I2C、OLED 和两路 SG90 PWM。
- 每 20ms 采样一次红外与触摸输入。
- 触摸优先：回中低头，并保持 1 秒触摸反馈。
- 左红外向左看，右红外向右看，两侧同时触发时回中。
- 手离开约 400ms 后缓慢回到默认姿势。
- 普通表情每 3 秒自动眨眼一次。

## 舵机

当前已加入 SG90 双舵机 LEDC 控制：

- 水平舵机：GPIO5
- 俯仰舵机：GPIO6
- PWM：50Hz
- 软件限幅和柔和起停
- 平滑控制逻辑有独立主机测试

SG90 参数和安全范围见 [docs/SG90_NOTES.md](docs/SG90_NOTES.md)。

主机端测试：

```powershell
gcc -std=c11 -Wall -Wextra -I main/include `
    main/src/servo_control.c tests/host/test_servo_control.c `
    -o tests/host/test_servo_control.exe
.\tests\host\test_servo_control.exe

gcc -std=c11 -Wall -Wextra -I main/include `
    main/src/behavior.c tests/host/test_behavior.c `
    -o tests/host/test_behavior.exe
.\tests\host\test_behavior.exe
```

## OLED 显示

OLED 在正式控制流程中显示机器人脸：

- 控制器：SSD1306 128x64
- I2C 地址：0x3C
- SDA：GPIO0
- SCL：GPIO1
- 上电显示两只圆角眼睛和笑脸
- 普通表情每 3 秒眨眼一次

OLED 接线：

| OLED 引脚 | ESP32-C3 |
| --- | --- |
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO0 |
| SCL | GPIO1 |

## 板载灯

- ESP32-C3 SuperMini 标准版蓝色用户灯：GPIO8
- 低电平点亮

标准版板载的是一颗单色蓝色用户灯，不是 RGB 灯。商家资料里的
`WS2812 ESP32C3.py` 用于外接 WS2812 灯带，不是板载蓝灯。

完整设计见 [docs/PROJECT_PLAN.md](docs/PROJECT_PLAN.md)。

## 硬件

- ESP32-C3 SuperMini
- 2 x SG90 舵机
- 0.96 寸 SSD1306 OLED
- TTP223 触摸模块
- 2 x 红外避障模块
- 外部稳压电源模块

## 开发环境

- ESP-IDF v6.1
- Visual Studio Code

## 编译

```powershell
idf.py set-target esp32c3
idf.py build
```

## 烧录

```powershell
idf.py -p COM端口 flash monitor
```

实际 COM 端口以设备管理器为准。

## 目录

```text
cardboard_robot/
├── CMakeLists.txt
├── sdkconfig.defaults
├── README.md
├── THIRD_PARTY_NOTICES.md
├── components/
│   └── ssd1306/
├── docs/
│   └── PROJECT_PLAN.md
├── main/
│   ├── CMakeLists.txt
│   ├── include/
│   │   ├── robot_app.h
│   │   ├── robot_config.h
│   │   ├── behavior.h
│   │   ├── input.h
│   │   ├── servo.h
│   │   └── display.h
│   └── src/
│       ├── robot_main.c
│       ├── robot_app.c
│       ├── behavior.c
│       ├── input.c
│       ├── servo.c
│       └── display.c
└── tests/
    └── host/
```

## 项目检查

```powershell
.\tools\check_project_structure.ps1
```

## 安全提示

舵机必须使用外部稳压 5V 电源，不能直接由 ESP32-C3 的 GPIO 或 3.3V 引脚供电。ESP32、舵机和所有模块必须共地。首次测试舵机时应卸下头壳或扶住头部，并先使用小角度范围。
