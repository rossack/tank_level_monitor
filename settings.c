#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/unique_id.h"
#include "hardware/watchdog.h"
#include "hardware/flash.h"
#include "flash_memory.h"
#include "utils.h"
#include "setting_defaults.h"
#include "settings.h"
#include "debug.h"

//
// Define settings structure with default values
//
/*
static SETTINGS_T settings = {
    0,
    WIFI_SSID,
    WIFI_PASSWORD,
    MQTT_SERVER_HOST,
    MQTT_SERVER_PORT,
    MQTT_USER,
    MQTT_PASSWORD,
    "",
    MQTT_STATE_TOPIC,
    MQTT_SENSOR_TOPIC,
    MQTT_CONFIG_TOPIC,
    DEF_PUB_INTERVAL,
    0
};
*/

static_assert(sizeof(SETTINGS_T) <= FLASH_PAGE_SIZE, "Settings struct too big for Flash");
static bool settings_changed = false;
static bool reboot_required = false;
static SETTINGS_T *settings;

//
// Get and Set functions
//

bool set_wifi_ssid(char * val) {
    if (strlen(val)> sizeof(settings->wifi_ssid)) return false;
    strncpy(settings->wifi_ssid,val,sizeof(settings->wifi_ssid));
    settings_changed = true;
    reboot_required = true;
    return true;
}

bool set_wifi_pwd(char * val) {
    if (strlen(val)> sizeof(settings->wifi_pwd)) return false;
    strncpy(settings->wifi_pwd,val,sizeof(settings->wifi_pwd));
    settings_changed = true;
    reboot_required = true;
    return true;
}

bool set_mqtt_host(char * val) {
    if (strlen(val)> sizeof(settings->mqtt_host)) return false;
    strncpy(settings->mqtt_host, val, sizeof(settings->mqtt_host));
    settings_changed = true;
    reboot_required = true;
    return true;
}

bool set_mqtt_port(uint val) {
    settings->mqtt_port = val;
    settings_changed = true;
    reboot_required = true;
    return true;
}


bool set_mqtt_user(char * val) {
    if (strlen(val)> sizeof(settings->mqtt_user)) return false;
    strncpy(settings->mqtt_user, val, sizeof(settings->mqtt_user));
    settings_changed = true;
    reboot_required = true;
    return true;
}
bool set_mqtt_pwd(char * val) {
    if (strlen(val)> sizeof(settings->mqtt_pwd)) return false;
    strncpy(settings->mqtt_pwd, val, sizeof(settings->mqtt_pwd));
    settings_changed = true;
    reboot_required = true;
    return true;
}

void set_mqtt_pint(uint val) {
    settings->mqtt_pint = val;
    settings_changed = true;
}

bool set_mqtt_state_topic(char * val) {
    if (strlen(val)> sizeof(settings->mqtt_state_topic)) return false;
    strncpy(settings->mqtt_state_topic, val, sizeof(settings->mqtt_state_topic));
    settings_changed = true;
    reboot_required = true;
    return true;
}
bool set_mqtt_sensor_topic(char * val) {
    if (strlen(val)> sizeof(settings->mqtt_sensor_topic)) return false;
    strncpy(settings->mqtt_sensor_topic, val, sizeof(settings->mqtt_sensor_topic));
    settings_changed = true;
    reboot_required = true;
    return true;
}
bool set_mqtt_config_topic(char * val) {
    if (strlen(val)> sizeof(settings->mqtt_config_topic)) return false;
    strncpy(settings->mqtt_config_topic, val, sizeof(settings->mqtt_config_topic));
    settings_changed = true;
    reboot_required = true;
    return true;
}



SETTINGS_T * get_settings(){
    return settings;
}



uint16_t calculate_checksum(const uint8_t *data, size_t length) {
    uint16_t checksum = 0;
    for (size_t i = 0; i < length; i++) {
        checksum += data[i]; // Add each byte to the checksum
    }
    return checksum;
}


//
// Initialise by getting settings from Flash memory
// If no values found in Flash, set the init_config flag and update Flash for next reboot
//
bool settings_init() {
    bool err;
    SETTINGS_T *flash_settings;
    settings = calloc(1, sizeof(SETTINGS_T));
    if (!settings) return ERR_ABRT;

    flash_settings = (SETTINGS_T *)flash_read();
    if (flash_settings == NULL) {
        DEBUG_printf("No settings. Booting in AP Mode.\n");
        // Insert a some default settings
        set_mqtt_state_topic(MQTT_STATE_TOPIC);
        set_mqtt_sensor_topic(MQTT_SENSOR_TOPIC);
        set_mqtt_config_topic(MQTT_CONFIG_TOPIC);
        return(false);
    } else {
        // copy flash contents to settings
        memcpy((char *)settings, flash_settings, sizeof(SETTINGS_T));
        // verify checksum
        // TBD
    }

    //
    // MQTT Client ID needs a unique value, set it if it's empty
    //
    if (strlen(settings->mqtt_cid)==0) {
        char pico_idstr[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];
        pico_get_unique_board_id_string(pico_idstr, sizeof(pico_idstr));
        sprintf(settings->mqtt_cid,"PicoW-%s", &pico_idstr[8]); // just use 2nd half of ID string
        DEBUG_printf("MQTT Client ID: %s\n", settings->mqtt_cid);
    }
    return true;
}


 void save_settings() {
    if (settings_changed) {
        // calculate and save the checksum
        // TBD
        flash_write((char *)settings, sizeof(SETTINGS_T));
        settings_changed = false;
    }
    // Most updates require a reboot
    // Allow some time to return from the Submit form
    //
    if (reboot_required) {
        // do deinit of LWIP resources ??
        watchdog_reboot(0, 0, 0);
    }
}

void reset_settings() {
    // This is called from either the long press RESET button (on GPIOxx) or
    // from the Web api when the user presses the big red Factory Reset button.
    // TBD: allow time back in main for the http respone to be sent?
    //
    settings_changed = false;
    flash_erase();
    watchdog_reboot(0, 0, 0);
}
