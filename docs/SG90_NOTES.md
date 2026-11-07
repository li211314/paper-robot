# SG90 舵机资料摘要

- 型号：TowerPro SG90 类微型 9g 位置舵机
- 控制方式：PWM
- PWM 频率：50Hz，周期约 20ms
- 公开示例常用脉宽：0.5ms 至 2.5ms，对应约 0 至 180 度
- 供电：常见规格为 4.8V 至 6.0V
- 本项目建议：外部稳压 5V 供电
- 反馈：无角度或位置反馈

参考资料：

- https://wiki.lckfb.com/zh-hans/dwx-cw32f030c8t6/module/control/sg90-steering-engine.html
- https://wiki.lckfb.com/zh-hans/dmx/module/control/sg90-steering-engine.html

安全设置：

- 水平舵机软件范围：1200us 至 1800us
- 俯仰舵机软件范围：1400us 至 1600us
- 不使用完整的 500us 至 2500us 范围，避免不同批次 SG90 或纸壳机构在端点卡住
- 首次测试必须空载或卸下头壳
- 出现堵转、持续嗡鸣、发热或纸壳干涉时立即断电

ESP32 与舵机必须共地。舵机电源不能取自 ESP32-C3 的 GPIO 或 3.3V 引脚。
