idf_component_register(SRCS "wifi.c" "spiffs.c" "i2s_acc.c" "main.c" "jpeg_http_stream.c"

                    INCLUDE_DIRS ".")
spiffs_create_partition_image(storage ../spiffs FLASH_IN_PROJECT)
