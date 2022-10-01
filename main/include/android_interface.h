#define CAM_CMD_ACTIVATE (char)0x12

int android_connect(void);
int android_disconnect(void);
bool is_packet_valid(void);
void android_send_udp(uint8_t *payload, int len);
void android_send_audio(uint8_t *payload, int len);


