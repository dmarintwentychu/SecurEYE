idf_component_register(SRCS "mqtt.c" "co2.c" "display.c" "getws.c" "getwp.c" "wifi.c" "spiffs.c" "main.c"
                    INCLUDE_DIRS ".")
spiffs_create_partition_image(storage ../spiffs FLASH_IN_PROJECT)
