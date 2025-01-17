


#define WIFI_SSID "PicoW_AP"
#define WIFI_PASSWORD "password"

// IP configuration for AP mode
#define AP_IP_ADDR "192.168.4.1"
#define AP_NETMASK "255.255.255.0"
#define AP_GATEWAY "192.168.4.1"

#define MQTT_SERVER_HOST "192.168.0.12"
#define MQTT_SERVER_PORT 1883
#define MQTT_USER "mquser"
#define MQTT_PASSWORD "password"

#define DEF_PUB_INTERVAL 0 
#define MQTT_STATE_TOPIC "tele/Water_Tank/STATE"
#if DEBUG_BUILD
#define MQTT_SENSOR_TOPIC "tele/Water_Tank/SENSOR_DBG"
#else
#define MQTT_SENSOR_TOPIC "tele/Water_Tank/SENSOR"
#endif
#define MQTT_CONFIG_TOPIC "cmnd/Water_Tank/CONFIG"

