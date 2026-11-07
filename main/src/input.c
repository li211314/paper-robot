#include "input.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "robot_config.h"

static const char *TAG = "robot_input";

typedef struct {
    bool initialized;
    bool stable;
    bool candidate;
    uint32_t candidate_since_ms;
} debounce_state_t;

typedef struct {
    debounce_state_t ir_left;
    debounce_state_t ir_right;
    debounce_state_t touch;
} input_state_t;

static input_state_t s_input_state;
static robot_inputs_t s_last_reported;

static bool level_is_active(int level, int active_level)
{
    /* 把物理电平转换成“功能是否触发”的布尔值。 */
    return level == active_level;
}

static bool debounce_update(debounce_state_t *state, bool raw, uint32_t now_ms)
{
    if (!state->initialized) {
        state->initialized = true;
        state->stable = raw;
        state->candidate = raw;
        state->candidate_since_ms = now_ms;
        return state->stable;
    }

    if (raw != state->candidate) {
        state->candidate = raw;
        state->candidate_since_ms = now_ms;
    }

    if (state->stable != state->candidate &&
        now_ms - state->candidate_since_ms >= ROBOT_INPUT_STABLE_MS) {
        state->stable = state->candidate;
    }
    return state->stable;
}

esp_err_t robot_input_init(void)
{
    /*
     * 常见红外避障模块是开漏/低电平有效，因此启用内部上拉。
     * 传感器断开时读为高电平，不会误判为检测到物体。
     */
    const gpio_config_t ir_config = {
        .pin_bit_mask = (1ULL << IR_LEFT_GPIO) | (1ULL << IR_RIGHT_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    /* TTP223 是推挽输出，模块内部已有输出级，不再启用上下拉。 */
    const gpio_config_t touch_config = {
        .pin_bit_mask = (1ULL << TOUCH_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t err = gpio_config(&ir_config);
    if (err != ESP_OK) {
        return err;
    }
    err = gpio_config(&touch_config);
    if (err == ESP_OK) {
        s_input_state = (input_state_t){0};
        ESP_LOGI(TAG, "Input initialized: IR active-low, touch active-high");
    }
    return err;
}

void robot_input_update(robot_inputs_t *inputs, uint32_t now_ms)
{
    if (!inputs) {
        return;
    }

    const bool left_raw = level_is_active(gpio_get_level(IR_LEFT_GPIO), IR_ACTIVE_LEVEL);
    const bool right_raw = level_is_active(gpio_get_level(IR_RIGHT_GPIO), IR_ACTIVE_LEVEL);
    const bool touch_raw = level_is_active(gpio_get_level(TOUCH_GPIO), TOUCH_ACTIVE_LEVEL);

    const robot_inputs_t next = {
        .ir_left = debounce_update(&s_input_state.ir_left, left_raw, now_ms),
        .ir_right = debounce_update(&s_input_state.ir_right, right_raw, now_ms),
        .touch = debounce_update(&s_input_state.touch, touch_raw, now_ms),
    };

    /* 防抖后的输入发生变化时才打印，避免 20ms 周期刷屏。 */
    if (next.ir_left != s_last_reported.ir_left ||
        next.ir_right != s_last_reported.ir_right ||
        next.touch != s_last_reported.touch) {
        ESP_LOGI(TAG, "input changed: left=%d right=%d touch=%d",
                 next.ir_left, next.ir_right, next.touch);
        s_last_reported = next;
    }

    *inputs = next;
}
