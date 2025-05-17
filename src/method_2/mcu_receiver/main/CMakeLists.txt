idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES logging manchester_encoding adc read_handler esp_timer protocol dfa)
