


#ifndef H_UTILS
#define H_UTILS

#define LED_PIN 22

#ifdef __cplusplus
extern "C" {
#endif

uint get_config_json(char *sbuf, uint sz);
uint get_sensor_json(char *sbuf, uint sz);
uint get_status_json(char *sbuf, uint sz);
float read_onboard_temperature();
void flashLED(int n);
void onLED();
void offLED();
void byteArrayToHexString(char *hexString, const uint8_t *byteArray, size_t length);

#ifdef __cplusplus
}
#endif

#endif