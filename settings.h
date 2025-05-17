
#ifndef H_SETTINGS
#define H_SETTINGS

#define CARR16 16 // up to 15 characters
#define CARR32 32 // up to 31 characters

typedef struct SETTINGS_T_ {
    uint16_t checksum;
    char  wifi_ssid[CARR16];
    char wifi_pwd[CARR16];
    char mqtt_host[CARR32];
    uint16_t mqtt_port;
    char mqtt_user[CARR16];
    char mqtt_pwd[CARR16];
    char mqtt_cid[CARR16];
    char mqtt_state_topic[CARR32];
    char mqtt_sensor_topic[CARR32];
    char mqtt_config_topic[CARR32];
    uint16_t mqtt_pint;
    uint16_t data_lint;

} SETTINGS_T;

//
// Functions to manage settings
// Values get saved to Flash memory
//
#ifdef __cplusplus
extern "C" {
#endif

bool set_wifi_ssid(char *);
bool set_wifi_pwd(char *);
void set_data_lint(uint);
bool set_mqtt_host(char *);
bool set_mqtt_port(uint);
bool set_mqtt_user(char *);
bool set_mqtt_pwd(char *);
void set_mqtt_pint(uint);
bool set_mqtt_state_topic(char *);
bool set_mqtt_sensor_topic(char *);
bool set_mqtt_config_topic(char *);

SETTINGS_T * get_settings();
bool settings_init();
void save_settings();
void reset_settings();

#ifdef __cplusplus
}
#endif

#endif