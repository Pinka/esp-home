

idf_component_register(SRCS "webserver.c"
                    INCLUDE_DIRS "."
                    EMBED_TXTFILES "certs/cacert.pem"
                                   "certs/prvtkey.pem"
                    REQUIRES "esp_https_server"
                             "json")
