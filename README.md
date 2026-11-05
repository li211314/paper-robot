# Cardboard Robot

基于 ESP32-C3 SuperMini 的小纸壳机器人。

项目使用 ESP-IDF v6.1 和 C 语言开发，计划实现 OLED 表情、两个 SG90 舵机、左右红外感应和触摸互动。

## 当前状态

方案设计阶段，尚未开始编写机器人程序。

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
├── docs/
│   └── PROJECT_PLAN.md
├── main/
│   └── 机器人 C 程序
├── CMakeLists.txt
├── sdkconfig.defaults
└── README.md
```

`main/` 和根目录的 CMake 配置将在下一阶段创建。

## 安全提示

舵机必须使用外部稳压 5V 电源，不能直接由 ESP32-C3 的 GPIO 或 3.3V 引脚供电。ESP32、舵机和所有模块必须共地。首次测试舵机时应卸下头壳或扶住头部，并先使用小角度范围。
