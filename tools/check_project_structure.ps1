Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot

$requiredFiles = @(
    "CMakeLists.txt",
    "sdkconfig.defaults",
    "main/CMakeLists.txt",
    "main/src/robot_main.c",
    "main/src/robot_app.c",
    "main/src/board_led.c",
    "main/src/behavior.c",
    "main/src/input.c",
    "main/src/servo.c",
    "main/src/servo_control.c",
    "main/src/display.c",
    "main/include/robot_config.h",
    "main/include/robot_types.h",
    "main/include/robot_app.h",
    "main/include/board_led.h",
    "main/include/behavior.h",
    "main/include/input.h",
    "main/include/servo.h",
    "main/include/servo_control.h",
    "main/include/display.h",
    "tests/host/test_servo_control.c",
    "tests/host/test_behavior.c",
    "components/ssd1306/CMakeLists.txt",
    "components/ssd1306/Kconfig.projbuild",
    "components/ssd1306/LICENSE",
    "components/ssd1306/ssd1306.c",
    "components/ssd1306/ssd1306.h",
    "components/ssd1306/ssd1306_i2c_new.c",
    "components/ssd1306/font8x8_basic.h"
)

$missing = @()
foreach ($relativePath in $requiredFiles) {
    $fullPath = Join-Path $root $relativePath
    if (-not (Test-Path -LiteralPath $fullPath -PathType Leaf)) {
        $missing += $relativePath
    }
}

if ($missing.Count -gt 0) {
    Write-Host "Project scaffold is incomplete." -ForegroundColor Red
    $missing | ForEach-Object { Write-Host "  missing: $_" }
    exit 1
}

$rootCMake = Get-Content -Raw (Join-Path $root "CMakeLists.txt")
$mainCMake = Get-Content -Raw (Join-Path $root "main/CMakeLists.txt")
$boardLedSource = Get-Content -Raw (Join-Path $root "main/src/board_led.c")
$servoSource = Get-Content -Raw (Join-Path $root "main/src/servo.c")
$servoControlSource = Get-Content -Raw (Join-Path $root "main/src/servo_control.c")
$robotMainSource = Get-Content -Raw (Join-Path $root "main/src/robot_main.c")
$robotAppSource = Get-Content -Raw (Join-Path $root "main/src/robot_app.c")
$inputSource = Get-Content -Raw (Join-Path $root "main/src/input.c")
$behaviorSource = Get-Content -Raw (Join-Path $root "main/src/behavior.c")
$displaySource = Get-Content -Raw (Join-Path $root "main/src/display.c")
$config = Get-Content -Raw (Join-Path $root "main/include/robot_config.h")
$flatComponentFiles = @(
    Get-ChildItem (Join-Path $root "main") -File |
        Where-Object Extension -In ".c", ".h"
)

$checks = @(
    @{
        Name = "root project name"
        Passed = $rootCMake -match "project\s*\(\s*cardboard_robot\s*\)"
    },
    @{
        Name = "minimal ESP-IDF build"
        Passed = $rootCMake -match "MINIMAL_BUILD\s+ON"
    },
    @{
        Name = "main component source discovery"
        Passed = $mainCMake -match "CMAKE_SCRIPT_MODE_FILE" -and
                 $mainCMake -match "file\s*\(\s*GLOB" -and
                 $mainCMake -match "CONFIGURE_DEPENDS" -and
                 $mainCMake -match "src/\*\.c" -and
                 $mainCMake -match "ssd1306" -and
                 $mainCMake -match 'INCLUDE_DIRS\s+"include"'
    },
    @{
        Name = "headers and sources separated"
        Passed = $flatComponentFiles.Count -eq 0
    },
    @{
        Name = "ESP32-C3 pin configuration"
        Passed = $config -match "OLED_SDA_GPIO\s*=\s*0" -and
                 $config -match "OLED_SCL_GPIO\s*=\s*1" -and
                 $config -match "IR_RIGHT_GPIO\s*=\s*3" -and
                 $config -match "IR_LEFT_GPIO\s*=\s*4" -and
                 $config -match "SERVO_PAN_GPIO\s*=\s*5" -and
                 $config -match "SERVO_TILT_GPIO\s*=\s*6" -and
                 $config -match "TOUCH_GPIO\s*=\s*10" -and
                 $config -match "BOARD_LED_GPIO\s*=\s*8" -and
                 $config -match "BOARD_LED_ACTIVE_LEVEL\s*=\s*0" -and
                 $config -notmatch "ROBOT_(LED|SERVO|DISPLAY)_BRINGUP_ONLY"
    },
    @{
        Name = "active-low GPIO LED driver"
        Passed = $boardLedSource -match "driver/gpio\.h" -and
                 $boardLedSource -match "BOARD_LED_ACTIVE_LEVEL" -and
                 $boardLedSource -notmatch "led_strip"
    },
    @{
        Name = "SG90 LEDC servo driver"
        Passed = $servoSource -match "driver/ledc\.h" -and
                 $servoSource -match "ledc_timer_config" -and
                 $servoSource -match "ledc_channel_config" -and
                 $servoSource -match "servo_motion_update" -and
                 $servoSource -notmatch "robot_servo_self_test" -and
                 ([regex]::Matches($servoSource, "esp_err_t robot_servo_init\(void\)")).Count -eq 1 -and
                 $servoSource -notmatch "s_targets"
    },
    @{
        Name = "application layer orchestration"
        Passed = $robotMainSource -match "robot_app_init" -and
                 $robotMainSource -match "robot_app_run" -and
                 $robotMainSource -notmatch "ROBOT_.*_BRINGUP_ONLY" -and
                 $robotAppSource -match "robot_input_update" -and
                 $robotAppSource -match "robot_behavior_update" -and
                 $robotAppSource -match "robot_servo_update" -and
                 $robotAppSource -match "robot_display_render"
    },
    @{
        Name = "portable servo control logic"
        Passed = $servoControlSource -match "servo_pulse_us_to_duty" -and
                 $servoControlSource -match "servo_motion_update" -and
                 $servoControlSource -match "ramp_step_us"
    },
    @{
        Name = "input debounce and behavior state"
        Passed = $inputSource -match "robot_input_update" -and
                 $inputSource -match "INPUT_STABLE" -and
                 $behaviorSource -match "touch_hold_until_ms" -and
                 $behaviorSource -match "last_ir_seen_ms" -and
                 $behaviorSource -match "blink_until_ms"
    },
    @{
        Name = "SSD1306 robot face rendering"
        Passed = $displaySource -match "i2c_master_init" -and
                 $displaySource -match "draw_robot_face" -and
                 $displaySource -match "ssd1306_show_buffer" -and
                 $displaySource -match "_ssd1306_line"
    }
)

$failed = @($checks | Where-Object { -not $_.Passed })
if ($failed.Count -gt 0) {
    Write-Host "Project scaffold content is invalid." -ForegroundColor Red
    $failed | ForEach-Object { Write-Host "  failed: $($_.Name)" }
    exit 1
}

Write-Host "Project scaffold structure is valid."
