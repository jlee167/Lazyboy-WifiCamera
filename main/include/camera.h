#include "esp_camera.h"

esp_err_t camera_init();
camera_fb_t* camera_capture();
void return_fb(camera_fb_t *fb);