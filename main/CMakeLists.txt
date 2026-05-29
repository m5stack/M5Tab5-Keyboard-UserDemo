set(FIRMWARE_VERSION "V1")
string(TIMESTAMP FIRMWARE_BUILD_TIME "%Y%m%d_%H%M%S")

idf_component_register(SRCS "tools.cpp" "main.cpp"
                            "ui_app.cpp"
                            "lcd_tools.cpp"
                            "usb_init.cpp"
                            "ui_pages/ui_page_base.cpp"
                            "ui_pages/ui_page_manager.cpp"
                            "ui_pages/ui_page_factory.cpp"
                            "ui_pages/ui_page_normal.cpp"
                            "ui_pages/ui_page_string.cpp"
                            "ui_pages/ui_page_hid.cpp"
                            "ui_pages/ui_page_registry.cpp"
                            "ui_pages/ui_top_menu.cpp"
                            "ui_pages/lvgl_keyboard_obj/lvgl_keyboard.cpp"
                            "ui_pages/lvgl_keyboard_obj/keyboards/factorytest_keyboard/factorytest_keyboard.cpp"
                            "ui_pages/lvgl_keyboard_obj/keyboards/m5tab5_keyboard/m5tab5_keyboard.cpp"
                    INCLUDE_DIRS "." "ui_pages" 
                                 "ui_pages/lvgl_keyboard_obj"
                                 "ui_pages/lvgl_keyboard_obj/keyboards/factorytest_keyboard"
                                 "ui_pages/lvgl_keyboard_obj/keyboards/m5tab5_keyboard"
                    REQUIRES esp_timer hal soc m5_tab5_component i2c_bus m5_tab5_keyboard_component esp_tinyusb nvs_flash esp_netif esp_event)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=missing-field-initializers)
target_compile_definitions(${COMPONENT_LIB} PRIVATE
        FIRMWARE_VERSION="${FIRMWARE_VERSION}"
        FIRMWARE_BUILD_TIME="${FIRMWARE_BUILD_TIME}"
)
