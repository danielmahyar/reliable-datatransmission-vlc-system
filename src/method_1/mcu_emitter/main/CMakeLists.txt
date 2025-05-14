idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES logging manchester_encoding light_handler esp_timer)
