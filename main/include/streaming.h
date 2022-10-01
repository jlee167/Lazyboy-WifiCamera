#include "esp_camera.h"

void init_http_client(char*);
void ping_server();
void send_image(char *web_token, char *url, camera_fb_t *fb);
void send_audio(void *buf, int len, char* url);