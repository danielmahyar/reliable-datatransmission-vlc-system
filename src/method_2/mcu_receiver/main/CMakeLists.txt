idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES logging manchester_encoding adc read_handler light_handler serial esp_timer protocol)
