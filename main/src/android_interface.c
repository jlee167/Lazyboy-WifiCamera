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
#include "lwip/sockets.h"
#include "lwip/errno.h"
#include <lwip/netdb.h>


static int sock;
static struct sockaddr_storage dest_addr;

static char rx_buffer[128];
static const char preamble[2] = {(char)0x24, (char)0x73};

int android_connect() {
    int err = 0;
    int rx_len = 0;

    if (sock = socket(AF_INET, SOCK_STREAM, 0) < 0) {
        return errno;
    }

    struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *) &dest_addr;
    dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr_ip4->sin_family = AF_INET;
    dest_addr_ip4->sin_port = htons(8001);
    err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in6));
    if (err) {
        android_disconnect();
    }

    send(sock, preamble, sizeof(preamble), 0);
    rx_len = recv(sock, rx_buffer, 2, 0);
    if (rx_buffer[0] == (char)0x24 && rx_buffer[1] == (char)0x73)
}


int android_disconnect() {
    close(sock);
}
