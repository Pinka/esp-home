

idf_component_register(SRCS "http.c"
                    INCLUDE_DIRS "."
                    REQUIRES "esp-tls" 
                             "esp_http_client"
                             "json")
