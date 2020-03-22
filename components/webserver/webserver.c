/* Simple HTTP + SSL Server 
*/

#include <esp_wifi.h>
#include <esp_log.h>
#include <esp_https_server.h>
#include <cJSON.h>

#include "webserver.h"

static const char *TAG = "webserver";

/* An HTTP GET handler */
static esp_err_t root_get_handler(httpd_req_t *req)
{
    int lower = 18;
    int upper = 30;

    cJSON *root, *temp;
    root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "address", cJSON_CreateString("Brivibas 118"));
    cJSON_AddItemToObject(root, "temp", temp = cJSON_CreateObject());
    cJSON_AddNumberToObject(temp, "zone1", (esp_random() % (upper - lower + 1)) + lower);
    cJSON_AddNumberToObject(temp, "zone2", (esp_random() % (upper - lower + 1)) + lower);
    cJSON_AddNumberToObject(temp, "zone3", (esp_random() % (upper - lower + 1)) + lower);
    cJSON_AddNumberToObject(temp, "zone4", (esp_random() % (upper - lower + 1)) + lower);

    char *response = cJSON_Print(root);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, response, -1); // -1 = use strlen()

    return ESP_OK;
}

static const httpd_uri_t root = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = root_get_handler};

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server");

    httpd_ssl_config_t conf = HTTPD_SSL_CONFIG_DEFAULT();

    extern const unsigned char cacert_pem_start[] asm("_binary_cacert_pem_start");
    extern const unsigned char cacert_pem_end[] asm("_binary_cacert_pem_end");
    conf.cacert_pem = cacert_pem_start;
    conf.cacert_len = cacert_pem_end - cacert_pem_start;

    extern const unsigned char prvtkey_pem_start[] asm("_binary_prvtkey_pem_start");
    extern const unsigned char prvtkey_pem_end[] asm("_binary_prvtkey_pem_end");
    conf.prvtkey_pem = prvtkey_pem_start;
    conf.prvtkey_len = prvtkey_pem_end - prvtkey_pem_start;

    esp_err_t ret = httpd_ssl_start(&server, &conf);
    if (ESP_OK != ret)
    {
        ESP_LOGI(TAG, "Error starting server!");
        return NULL;
    }

    // Set URI handlers
    ESP_LOGI(TAG, "Registering URI handlers");
    httpd_register_uri_handler(server, &root);
    return server;
}

static void stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    httpd_ssl_stop(server);
}

static void disconnect_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    httpd_handle_t *server = (httpd_handle_t *)arg;
    if (*server)
    {
        stop_webserver(*server);
        *server = NULL;
    }
}

static void connect_handler(void *arg, esp_event_base_t event_base,
                            int32_t event_id, void *event_data)
{
    httpd_handle_t *server = (httpd_handle_t *)arg;
    if (*server == NULL)
    {
        *server = start_webserver();
    }
}

void webserver_init()
{
    static httpd_handle_t server = NULL;

    /* Register event handlers to start server when Wi-Fi or Ethernet is connected,
     * and stop server when disconnection happens.
     */
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));
}
