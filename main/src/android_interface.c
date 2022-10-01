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

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/errno.h"
#include <lwip/netdb.h>

#include "android_interface.h"
#include "types.h"

static int sock;
static int video_sock;
static int audio_sock;

static struct sockaddr_storage dest_addr;
static struct sockaddr_storage udp_video_addr;
static struct sockaddr_storage udp_audio_addr;

static char rx_buffer[128];
static char tx_buffer[128];

extern network_packet video_url;
extern network_packet audio_url;
extern network_packet geo_url;
extern network_packet access_token;

static const char preamble[2] = {(char)0x24, (char)0x73};

const char *test_serial = "123456789"; 




int android_connect(void) {
    int err = 0;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return errno;
    }
    struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *) &dest_addr;
    dest_addr_ip4->sin_addr.s_addr = inet_addr("192.168.43.1");
    dest_addr_ip4->sin_family = AF_INET;
    dest_addr_ip4->sin_port = htons(8001);
    err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in6));
    if (err) {
        printf("[ERROR] Android Connection failed! Error Code: %d\n", err);
        android_disconnect();
        return err;
    }
      
    /* Send connection request to Android */
    memcpy((void*) tx_buffer, (void*) preamble, sizeof(preamble));
    tx_buffer[2] = CAM_CMD_ACTIVATE;
    memcpy((void*) &tx_buffer[3], (void*) test_serial, 9);
    send(sock, tx_buffer, 12, 0);

    printf("[Android] Android connection successful!\n");  
    
    recv(sock, (char*) &video_url.length, 1, 0);
    printf("recv");
    printf("%d", video_url.length);
    video_url.data = malloc(video_url.length+1);
    recv(sock, video_url.data, video_url.length, 0);
    video_url.data[video_url.length] = '\0';
    printf("[Android] Video URL = %s\n", video_url.data);


    recv(sock, (char*) &audio_url.length, 1, 0);
    audio_url.data = malloc(audio_url.length+1);
    recv(sock, audio_url.data, audio_url.length, 0);
    audio_url.data[audio_url.length] = '\0';
    printf("[Android] Audio URL = %s\n", audio_url.data);

    recv(sock, (char*) &geo_url.length, 1, 0);
    geo_url.data = malloc(geo_url.length+1);
    recv(sock, geo_url.data, geo_url.length, 0);
    geo_url.data[geo_url.length] = '\0';
    printf("[Android] Geo URL = %s\n", geo_url.data);

    recv(sock, (char*) &access_token.length, 1, 0);
    access_token.data = malloc(access_token.length+1);
    recv(sock, access_token.data, access_token.length, 0);
    access_token.data[access_token.length] = '\0';
    printf("[Android] Access Token = %s\n", access_token.data);


    if ((video_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        return errno;
    }
    struct sockaddr_in *video_addr_ip4 = (struct sockaddr_in *) &udp_video_addr;
    video_addr_ip4->sin_addr.s_addr = inet_addr("192.168.43.1");
    video_addr_ip4->sin_family = AF_INET;
    video_addr_ip4->sin_port = htons(8002);


    if ((audio_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        return errno;
    }
    struct sockaddr_in *audio_addr_ip4 = (struct sockaddr_in *) &udp_audio_addr;
    audio_addr_ip4->sin_addr.s_addr = inet_addr("192.168.43.1");
    audio_addr_ip4->sin_family = AF_INET;
    audio_addr_ip4->sin_port = htons(8003);


    //err = connect(sock, (struct sockaddr *)&udp_video_addr, sizeof(struct sockaddr_in6));
    if (err) {
        printf("[ERROR] UDP Connection failed! Error Code: %d\n", err);
        android_disconnect();
        return err;
    }

    return 0;
}


void android_send_udp(uint8_t *payload, int len) {
    int sent = sendto(video_sock, (char*)payload, len, MSG_DONTWAIT,
                (struct sockaddr *)&udp_video_addr, sizeof(struct sockaddr_in6));
    if (sent != -1) {
        printf("[VIDEO] SENT: %d bytes\n", sent);
    } else {
        printf("[VIDEO] Send error: code %d \n", errno);
    }
}


void android_send_audio(uint8_t *payload, int len) {
    int sent = sendto(audio_sock, (char*)payload, len, MSG_DONTWAIT,
                (struct sockaddr *)&udp_audio_addr, sizeof(struct sockaddr_in6));
    if (sent != -1) {
        printf("[AUDIO] Sent: %d bytes\n", sent);
    } else {
        printf("[AUDIO] Send error: code %d \n", errno);
    }
}


bool is_packet_valid(void) {
    return rx_buffer[0] == preamble[0] 
        && rx_buffer[1] == preamble[1];
}


int android_disconnect(void) {
    int result = close(sock);
    return result;
}
