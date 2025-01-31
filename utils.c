
#include <stdio.h>
#include <stdarg.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/adc.h"
#include "settings.h"
#include "utils.h"

uint get_config_json(char *sbuf, uint sz) {
    snprintf(sbuf, sz, "{\"ssid\": \"%s\","
    "\"pwd\": \"%s\","
    "\"mqHost\": \"%s\","
    "\"mqPort\": %d,"
    "\"mqUser\": \"%s\","
    "\"mqPwd\": \"%s\","
    "\"mqStateTopic\": \"%s\","
    "\"mqSensTopic\": \"%s\","
    "\"mqConfTopic\": \"%s\","
    "\"mqPubInt\": %d}",
      get_settings()->wifi_ssid,
      get_settings()->wifi_pwd,
      get_settings()->mqtt_host,
      get_settings()->mqtt_port,
      get_settings()->mqtt_user,
      get_settings()->mqtt_pwd,
      get_settings()->mqtt_state_topic,
      get_settings()->mqtt_sensor_topic,
      get_settings()->mqtt_config_topic,
      get_settings()->mqtt_pint);
    return strlen(sbuf);
}

uint get_sensor_json(char *sbuf, uint sz) {
    adc_select_input(0); // Select ADC input 0 (GPIO26)
    /* 12 bit converter 0 to 4095 0x0fff */
    const float conversion_factor = 3.3f / (1 << 12);
    uint16_t raw = adc_read();
    snprintf(sbuf, sz, "{\"ADC\": %u, \"Hex\": \"%04x\", \"Voltage\": %.3f}",
    raw, raw, (float)raw * conversion_factor);
    return strlen(sbuf);
}

uint get_status_json(char *sbuf, uint sz) {
    int32_t rssi;
    cyw43_wifi_get_rssi(&cyw43_state, &rssi);
    uint8_t mac_address[6];
    cyw43_wifi_get_mac(&cyw43_state, CYW43_ITF_AP, mac_address);
    struct netif *myNetif = netif_default;
    ip_addr_t ipaddr = myNetif->ip_addr;

    snprintf(sbuf, sz,
    "{\"Version\": \"%s\","
    "\"Temp\": %.1f,"
    "\"UptimeSec\": %u,"
    "\"RSSI\": %d,"
    "\"SSID\": \"%s\","
    "\"IPAddr\": \"%s\","
    "\"MAC\": \"%02X:%02X:%02X:%02X:%02X:%02X\"}",
      BUILD_VERSION,
      read_onboard_temperature(),
      to_ms_since_boot(get_absolute_time())/1000,
      rssi,
      get_settings()->wifi_ssid,
      ip4addr_ntoa(&ipaddr),
      mac_address[0], mac_address[1], mac_address[2],
      mac_address[3], mac_address[4], mac_address[5]);
    return strlen(sbuf);
}



float read_onboard_temperature() {
    adc_select_input(4);
    /* 12-bit conversion, assume max value == ADC_VREF == 3.3 V */
    const float conversionFactor = 3.3f / (1 << 12);
    float adc = (float)adc_read() * conversionFactor;
    float tempC = 27.0f - (adc - 0.706f) / 0.001721f;
    return tempC;
}


void flashLED(int n) {
    int i=0;
    while (i < n) {
        //
        // Flash the on board LED and the external LED
        //
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        gpio_put(LED_PIN, 1);
        sleep_ms(100);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        gpio_put(LED_PIN, 0);
        if (++i < n) sleep_ms(100); //no need to sleep on last flash
    }
}

void onLED() {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    gpio_put(LED_PIN, 1);
}
void offLED() {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
    gpio_put(LED_PIN, 0);
}

void byteArrayToHexString(char *hexString, const uint8_t *byteArray, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        sprintf(hexString + (i * 2), "%02X", byteArray[i]);
    }
    hexString[length * 2] = '\0'; // Null-terminate the string
}