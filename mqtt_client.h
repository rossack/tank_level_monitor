

#ifndef H_MQTT_CLIENT
#define H_MQTT_CLIENT

#ifdef __cplusplus
extern "C" {
#endif

err_t mqtt_client_init();
err_t mqtt_update();
u32_t getPubCount();
u32_t get_mqtt_json(char *sbuf, uint sz);

#ifdef __cplusplus
}
#endif

#endif