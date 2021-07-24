#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "driver/gpio.h"
#include "driver/i2s.h"
#include "esp_camera.h"


#define AUDIO_I2S_CHANNEL   1


static const i2s_pin_config_t pin_config = {
    .bck_io_num = 15,
    .ws_io_num = 4,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = 2
};


static const i2s_config_t i2s_config = {
    .mode = I2S_MODE_MASTER | I2S_MODE_RX,
    .sample_rate = 48000,//44100,
    .bits_per_sample = 24,//16,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = 0, // default interrupt priority
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false
};

void audio_init(){
    i2s_set_pin(AUDIO_I2S_CHANNEL, &pin_config);
    i2s_driver_install(AUDIO_I2S_CHANNEL, &i2s_config, 0, NULL);
}



void audio_pause(){
    i2s_stop(AUDIO_I2S_CHANNEL);
}

void audio_resume(){
    i2s_start(AUDIO_I2S_CHANNEL);
}

void audio_stop(){
     i2s_driver_uninstall(AUDIO_I2S_CHANNEL);
}