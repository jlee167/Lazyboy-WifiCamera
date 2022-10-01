#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "esp_http_client.h"

#include "driver/gpio.h"
#include "esp_camera.h"

#include "streaming.h"


bool conn_mutex = false;

esp_http_client_config_t config;
esp_http_client_handle_t client;


char *TAG = "[HTTP CLIENT] ";

esp_err_t _http_event_handle(esp_http_client_event_t *evt) {
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            //ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            //ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            //ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            //ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER");
            //printf("%.*s", evt->data_len, (char*)evt->data);
            break;
        case HTTP_EVENT_ON_DATA:
            //ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (!esp_http_client_is_chunked_response(evt->client)) {
                //printf("%.*s", evt->data_len, (char*)evt->data);
            }
            conn_mutex = false;
            break;
        case HTTP_EVENT_ON_FINISH:
            //ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            //ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
    }
    
    return ESP_OK;
}


void init_http_client(char* web_token) {
    config.url = "http://www.lazyboyindustries.com:3001";
    config.event_handler = _http_event_handle;
    client = esp_http_client_init(&config);
    //esp_http_client_set_header(client, "webToken", web_token);
    //esp_err_t http_err = esp_http_client_perform(http_client);
}

void ping_server() {
    if (!conn_mutex) {
        conn_mutex = true;
        ESP_LOGI(TAG, "Pinging Server!\n");
        client = esp_http_client_init(&config);
        esp_http_client_set_url(client, "http://www.lazyboyindustries.com:3001/ping");
        esp_http_client_set_method(client, HTTP_METHOD_GET);
        esp_err_t err = esp_http_client_perform(client);
        esp_http_client_cleanup(client);
    }
}


void send_image(char *web_token, char *url, camera_fb_t *fb) {

    static bool client_ready = false;

    if (!client_ready) {
        config.url = url;
        config.event_handler = _http_event_handle;
        client = esp_http_client_init(&config);
        esp_http_client_set_url(client, url);
        esp_http_client_set_method(client, HTTP_METHOD_POST);
        esp_http_client_set_header(client, "webToken", web_token);
        esp_http_client_set_header(client, "Content-Type", "application/octet-stream");
        client_ready = true;
    }
    esp_http_client_open(client, fb->len);
    esp_http_client_write(client, (const char *)fb->buf, fb->len);
    esp_http_client_close(client);
    //esp_http_client_cleanup(client);
}


void send_audio(void *buf, int len, char* url) {
    char *data = malloc(len + 9);
    strcpy(data, "{\"data\":\""); 
    memcpy(&data[9], (char*)buf, len);
    strcpy(&data[9 + len], "\"}");
    esp_http_client_set_url(client, url);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/octet-stream");
}