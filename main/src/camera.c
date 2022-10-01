
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "driver/gpio.h"
#include "esp_camera.h"

/* Lazyboy Camera Setting */
#define CAM_PIN_PWDN -1  //power down is not used
#define CAM_PIN_RESET -1 //software reset will be performed
#define CAM_PIN_XCLK 0
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27

#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 21
#define CAM_PIN_D2 19
#define CAM_PIN_D1 18
#define CAM_PIN_D0 5
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22

#define HIGH 1
#define LOW 0

static const char *TAG = "camera";

static camera_config_t camera_config = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sscb_sda = CAM_PIN_SIOD,
    .pin_sscb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    .xclk_freq_hz = 20000000, //EXPERIMENTAL: Set to 16MHz on ESP32-S2 or ESP32-S3 to enable EDMA mode
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_VGA,    //FRAMESIZE_UXGA,   //QQVGA-QXGA Do not use sizes above QVGA when not JPEG

    .jpeg_quality = 12,                 //0-63 lower number means higher quality
    .fb_count = 3,                      //if more than one, i2s runs in continuous mode. Use only with JPEG
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY //CAMERA_GRAB_LATEST. Sets when buffers should be filled
};

esp_err_t camera_init()
{

    //power up the camera if PWDN pin is defined

    gpio_config_t io_led_conf;
    io_led_conf.mode = GPIO_MODE_OUTPUT;
    io_led_conf.pull_down_en = false;
    io_led_conf.pull_up_en = false;
    io_led_conf.pin_bit_mask = (GPIO_SEL_32);
    gpio_config(&io_led_conf);

    //gpio_set_direction(CAM_PIN_PWDN, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_32, HIGH);
    ESP_LOGE(TAG, "Camera Pin Powered");

    //initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Camera Init Failed");
        return err;
    }

    return ESP_OK;
}

camera_fb_t* camera_capture()
{
    /* acquire a frame */
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb)
    {
        ESP_LOGE(TAG, "Camera Capture Failed");
    }
    return fb;

    /* replace this with your own function */
    //process_image(fb->width, fb->height, fb->format, fb->buf, fb->len);
}

void return_fb(camera_fb_t *fb)
{
    /* return the frame buffer back to the driver for reuse */
    esp_camera_fb_return(fb);
}
