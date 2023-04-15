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
#include "esp_intr_alloc.h"

#include "camera.h"
#include "wifi.h"
#include "android_interface.h"
#include "streaming.h"
#include "stream_info.h"
#include "config.h"
#include "audio.h"
#include "types.h"
#include "settings.h"

#define CAPTURE_BUTTON GPIO_NUM_13

#define CORE_0  (0)
#define CORE_1  (1)

volatile int debounce = 0;
static bool capture_pressed = false;
static bool wifi_connected;
static bool android_connected;
static bool cam_active;

static void worker_io(void *p);
static void worker_video(void *p);
static void worker_audio(void *p);
static void worker_audio_send(void *p);

static void stabilize_gpio(int gpio_num);
static void setup_gpio(void);
static void isr_btn_click(void *arg);
void notify_connection(bool success);

static camera_fb_t *fb;

network_packet video_url;
network_packet audio_url;
network_packet geo_url;
network_packet access_token;

TaskHandle_t io_task_handle;
TaskHandle_t video_task_handle;
TaskHandle_t audio_task_handle;
TaskHandle_t audio_send_task_handle;

UBaseType_t io_task_priority;
UBaseType_t video_task_priority;
UBaseType_t audio_task_priority;
UBaseType_t audio_send_task_priority;

int audio_buf_tail = 0; 
int audio_buf_head = 0;
char buf_i2s[3][8*1024];

bool running = false;




/**
 * @brief Handle button press events. 
 *        Debouncing logicss: counter, interrupt disabling  
 * @param arg 
 */
static void isr_btn_click(void *arg)
{
    if (debounce > DEBOUNCE_ITER_MIN)
    {
        debounce = 0;
        capture_pressed = true;
        gpio_intr_disable(GPIO_NUM_13);

        /* Notify GPIO task to run once */
        BaseType_t xHigherPriorityTaskWoken = pdTRUE;
        vTaskNotifyGiveFromISR(io_task_handle, &xHigherPriorityTaskWoken);
    }
    else
    {
        debounce++;
    }
}




/**
 * @brief Initialize app states
 * 
 */
static void init_flags()
{
    wifi_connected = false;
    android_connected = false;
    cam_active = false;
}




void notify_connection(bool success)
{
    wifi_connected = success;
}




/**
 * @brief Initialize general purpose IO ports
 * 
 */
static void setup_gpio(void)
{
    gpio_config_t start_btn_conf;
    start_btn_conf.intr_type = GPIO_PIN_INTR_NEGEDGE;
    start_btn_conf.mode = GPIO_MODE_INPUT;
    start_btn_conf.pull_down_en = false;
    start_btn_conf.pull_up_en = false;
    start_btn_conf.pin_bit_mask = (GPIO_SEL_13);
    gpio_config(&start_btn_conf);

    gpio_config_t io_led_conf;
    io_led_conf.mode = GPIO_MODE_OUTPUT;
    io_led_conf.pull_down_en = false;
    io_led_conf.pull_up_en = false;
    io_led_conf.pin_bit_mask = (GPIO_SEL_14);
    gpio_config(&io_led_conf);

    stabilize_gpio(GPIO_NUM_13);
    gpio_set_level(GPIO_NUM_14, 0);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(CAPTURE_BUTTON, isr_btn_click, (void *)CAPTURE_BUTTON);
    gpio_intr_enable(GPIO_NUM_13);
}


static void stabilize_gpio(int gpio_num)
{
    int stabilize_cnt = 0;
    while (stabilize_cnt < 3000) {
        if (gpio_get_level(GPIO_NUM_13))
            stabilize_cnt++;
        else
            stabilize_cnt = 0;
    }
}




/**
 * @brief 
 * 
 * @param fb_p 
 */
static void process_image(camera_fb_t *fb_p)
{
    send_image(access_token.data, video_url.data, fb_p);
    return_fb(fb_p);
}




/**
 * @brief Task worker handling button press event
 * 
 * @param p 
 */
static void worker_io(void *p) {
    
    printf("[TASK] Starting IO Task\n");

    while (true) {
        /* Wait indefenitely for GPIO Interrupt notification*/
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        printf("Capture Button Pressed\n");

        running = !running;
        stabilize_gpio(GPIO_NUM_13);
        gpio_intr_enable(GPIO_NUM_13);
        capture_pressed = false;
    }
}




/**
 * @brief 
 * 
 * @param p 
 */
static void worker_video(void *p)
{
    printf("[TASK] Video Sample Start\n");

    while(true) {
        if (running)
        {
            static int frame_cnt = 0;
            fb = camera_capture();
            if(fb) {
                frame_cnt++;
                printf("Frame Number: %d\n", frame_cnt);
                //send_image(access_token.data, video_url.data, fb);
                //android_send_udp((uint8_t *)buf_i2s, 1024);
                android_send_udp(fb->buf, fb->len);
                
                return_fb(fb);
            }
        }
    }
}




/**
 * @brief 
 * 
 * @param p
 */
static void worker_audio(void *p) {
    printf("[TASK] Audio Sample Start\n");

    while (true) {
        if (running) {          
            audio_read(buf_i2s[audio_buf_head], 8*1024);

            if (audio_buf_head < 2)
                audio_buf_head++;
            else
                audio_buf_head = 0;
        }
    }
}


static void worker_audio_send(void *p) {
    printf("[TASK] Starting Audio Send Task\n");

    while (true) {
        if (running) {
            if (audio_buf_head == audio_buf_tail)
                continue;

            android_send_audio((uint8_t *)buf_i2s[audio_buf_tail], 8*1024);
            if (audio_buf_tail < 2)
                audio_buf_tail++;
            else
                audio_buf_tail = 0;
        }
    }
}



static void setup_camera()
{
    while (!cam_active)
    {
        esp_err_t err = camera_init();
        if (err == ESP_OK)
        {
            cam_active = true;
        };
    }
}


 
void app_main(void)
{
    /* Task Priority Settings */
    io_task_priority = tskIDLE_PRIORITY + 1;
    video_task_priority = tskIDLE_PRIORITY; 
    audio_task_priority = tskIDLE_PRIORITY;
    audio_send_task_priority = tskIDLE_PRIORITY;


    init_flags();
    setup_gpio();
    init_wifi();
    setup_camera();
    audio_init();
    audio_resume();
    
    printf("Peripheral setup done!\n");
    android_connect();  
    
    xTaskCreatePinnedToCore(
        worker_video,
        "worker_video",
        8000,
        NULL,
        video_task_priority,
        &video_task_handle,
        CORE_0
    );


    xTaskCreatePinnedToCore(
        worker_io,
        "worker_io",
        1024,
        NULL,
        io_task_priority,
        &io_task_handle,
        CORE_0
    );


    xTaskCreatePinnedToCore(
        worker_audio,
        "worker_audio",
        10000,
        NULL,
        audio_task_priority,
        &audio_task_handle,
        CORE_1
    );


    xTaskCreatePinnedToCore(
        worker_audio_send,
        "worker_audio_send",
        6000,
        NULL,
        audio_send_task_priority,
        &audio_send_task_handle,
        CORE_1
    );   
}
