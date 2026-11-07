if(CMAKE_SCRIPT_MODE_FILE)
    file(GLOB MAIN_SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/src/*.c"
    )
else()
    file(GLOB MAIN_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_LIST_DIR}/src/*.c"
    )
endif()

idf_component_register(
    SRCS ${MAIN_SOURCES}
    INCLUDE_DIRS "include"
    PRIV_REQUIRES
        esp_driver_gpio
        esp_driver_i2c
        esp_driver_ledc
        esp_timer
        ssd1306
)
