idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES bsp_waveshare_7b esp_timer esp_psram esp_hw_support
)