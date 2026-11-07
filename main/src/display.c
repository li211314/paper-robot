#include "display.h"

#include "esp_log.h"
#include "robot_config.h"
#include "ssd1306.h"

static const char *TAG = "robot_display";
static SSD1306_t s_display;
static bool s_ready;
static uint8_t s_blank_buffer[OLED_WIDTH * OLED_HEIGHT / 8];

static const char *expression_name(robot_expression_t expression)
{
    switch (expression) {
    case ROBOT_EXPRESSION_CLOSED:
        return "closed";
    case ROBOT_EXPRESSION_NEAR:
        return "near";
    case ROBOT_EXPRESSION_TOUCHED:
        return "touched";
    case ROBOT_EXPRESSION_NORMAL:
    default:
        return "normal";
    }
}

/*
 * 驱动中的 _ssd1306_* 函数只修改芯片内的软件显存，不立即发送到屏幕。
 * 一张脸画完后统一调用 ssd1306_show_buffer 刷新，减少 I2C 小包传输。
 */
static void draw_thick_line(SSD1306_t *dev,
                            const int points[][2],
                            int point_count,
                            int thickness,
                            bool invert)
{
    /* 通过多次上下偏移画线来模拟 2~3 像素的笔画。 */
    for (int offset = 0; offset < thickness; offset++) {
        for (int i = 0; i < point_count - 1; i++) {
            _ssd1306_line(dev,
                          points[i][0],
                          points[i][1] + offset,
                          points[i + 1][0],
                          points[i + 1][1] + offset,
                          invert);
        }
    }
}

static void draw_rounded_eye(SSD1306_t *dev,
                             int x,
                             int y,
                             int width,
                             int height)
{
    /* 用逐行横线拼出圆角矩形；数组表示上下 8 行逐行向内的像素数。 */
    static const int insets[8] = {8, 6, 4, 3, 2, 1, 1, 0};

    for (int row = 0; row < height; row++) {
        int inset = 0;
        if (row < 8) {
            inset = insets[row];
        } else if (height - 1 - row < 8) {
            inset = insets[height - 1 - row];
        }
        if (width - 2 * inset > 0) {
            _ssd1306_line(dev,
                          x + inset,
                          y + row,
                          x + width - 1 - inset,
                          y + row,
                          false);
        }
    }
}

static void draw_eye_ring(SSD1306_t *dev, int center_x, int center_y)
{
    /* 先画实心圆，再用反色圆挖空，形成粗眼环和小高光。 */
    _ssd1306_disc(dev, center_x, center_y, 12, OLED_DRAW_ALL, false);
    _ssd1306_disc(dev, center_x, center_y, 7, OLED_DRAW_ALL, true);
    _ssd1306_disc(dev, center_x - 5, center_y - 5, 2, OLED_DRAW_ALL, false);
}

static void draw_normal_face(SSD1306_t *dev)
{
    /* 128x64 屏幕坐标：左上角为 (0,0)，右下角为 (127,63)。 */
    static const int smile[][2] = {
        {54, 48}, {58, 52}, {64, 53}, {70, 52}, {74, 48},
    };

    draw_rounded_eye(dev, 26, 10, 28, 34);
    draw_rounded_eye(dev, 74, 10, 28, 34);
    _ssd1306_disc(dev, 35, 20, 3, OLED_DRAW_ALL, true);
    _ssd1306_disc(dev, 83, 20, 3, OLED_DRAW_ALL, true);
    draw_thick_line(dev, smile, 5, 2, false);
}

static void draw_blink_face(SSD1306_t *dev)
{
    /* 闭眼时用两条 3 像素粗的横线代替眼睛。 */
    static const int smile[][2] = {
        {54, 48}, {58, 52}, {64, 53}, {70, 52}, {74, 48},
    };

    for (int offset = 0; offset < 3; offset++) {
        _ssd1306_line(dev, 28, 29 + offset, 51, 29 + offset, false);
        _ssd1306_line(dev, 76, 29 + offset, 99, 29 + offset, false);
    }
    draw_thick_line(dev, smile, 5, 2, false);
}

static void draw_near_face(SSD1306_t *dev)
{
    /* 靠近表情使用空心圆眼和更明显的笑脸。 */
    static const int smile[][2] = {
        {55, 43}, {59, 47}, {64, 48}, {69, 47}, {73, 43},
    };

    draw_eye_ring(dev, 40, 29);
    draw_eye_ring(dev, 88, 29);
    draw_thick_line(dev, smile, 5, 3, false);
}

static void draw_touched_face(SSD1306_t *dev)
{
    /* 摸头表情使用半闭眼和向下弯的嘴。 */
    static const int frown[][2] = {
        {54, 46}, {58, 42}, {64, 41}, {70, 42}, {74, 46},
    };

    draw_rounded_eye(dev, 28, 20, 24, 13);
    draw_rounded_eye(dev, 76, 20, 24, 13);
    draw_thick_line(dev, frown, 5, 2, false);
}

static void draw_robot_face(SSD1306_t *dev, robot_expression_t expression)
{
    switch (expression) {
    case ROBOT_EXPRESSION_CLOSED:
        draw_blink_face(dev);
        break;
    case ROBOT_EXPRESSION_NEAR:
        draw_near_face(dev);
        break;
    case ROBOT_EXPRESSION_TOUCHED:
        draw_touched_face(dev);
        break;
    case ROBOT_EXPRESSION_NORMAL:
    default:
        draw_normal_face(dev);
        break;
    }
}

esp_err_t robot_display_init(void)
{
    /* 标准版 Super Mini 使用 GPIO0/1 作为软件 I2C，和板载灯/BOOT 引脚错开。 */
    i2c_master_init(&s_display, OLED_SDA_GPIO, OLED_SCL_GPIO, -1);

    esp_err_t err = ssd1306_init(&s_display, OLED_WIDTH, OLED_HEIGHT);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SSD1306 initialization failed: %s", esp_err_to_name(err));
        return err;
    }

    s_display._flip = false;  /* 屏幕上下颠倒时改为 true */
    ssd1306_clear_screen(&s_display, false);
    ssd1306_contrast(&s_display, 0x7F);
    s_ready = true;

    ESP_LOGI(TAG, "SSD1306 ready on SDA=%d SCL=%d",
             OLED_SDA_GPIO, OLED_SCL_GPIO);
    return ESP_OK;
}

void robot_display_render(robot_expression_t expression)
{
    if (!s_ready) {
        return;
    }

    /*
     * 只清软件显存，不立即写 I2C。
     * 表情画完后统一 show 一次，减少对 20ms 舵机控制循环的阻塞。
     */
    ssd1306_set_buffer(&s_display, s_blank_buffer);
    draw_robot_face(&s_display, expression);
    ssd1306_show_buffer(&s_display);
    ESP_LOGI(TAG, "rendered expression=%s", expression_name(expression));
}
