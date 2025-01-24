#include <stdio.h>
#include <stdarg.h>
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/watchdog.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/binary_info.h"
#include "lwip/tcpip.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/dhcp.h"
#include "lwip/timeouts.h"

#include "lwipopts.h"

#include "mqtt_client.h"
#include "dhcpserver/dhcpserver.h"
#include "httpserver/httpserver.h"
#include "button.h"
#include "setting_defaults.h"
#include "settings.h"
#include "flash_memory.h"
#include "utils.h"
#include "debug.h"

#define MPB_PIN 13 // GPIO pin to Momentary Push Button
#define ADC_PIN 26 // ADC0 GPIO pin

MPButton mpButton; // Momentary Push button
volatile bool reset_settings_flag = false;

void extInterrupt(uint gpio, uint32_t hw_e) {
    if (gpio == MPB_PIN) {
        mpButton.interrupt(hw_e); // notify the button object
    } else DEBUG_printf("Unknown interrupt on gpio: %u", gpio);
}

void buttonEvent(Event e) {
    DEBUG_printf("Button Event:%d\n",e);
    // We are inside an interrupt here so dont do anything fancy
    // Let the ilde loop do the work
    switch (e) {
        case CLICK:
            break;
        case DBCLICK:
        break;
        case LPRESS:
            reset_settings_flag = true;
            break;
    }
}

 void setup_ap_mode() {

    cyw43_arch_enable_ap_mode(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK);
    DEBUG_printf("AP Mode SSID: %s\n", WIFI_SSID);

    // Set static IP for the AP
    ip4_addr_t ipaddr, netmask, gateway;
    ip4addr_aton(AP_IP_ADDR, &ipaddr);
    ip4addr_aton(AP_NETMASK, &netmask);
    ip4addr_aton(AP_GATEWAY, &gateway);
    /*
    IP4_ADDR(&ipaddr, 192, 168, 4, 1);  // AP IP address
    IP4_ADDR(&netmask, 255, 255, 255, 0); // Subnet mask
    IP4_ADDR(&gateway, 192, 168, 4, 1);        // Gateway
    */

    struct netif *netif;
    // Iterate over netif interfaces
    for (netif = netif_list; netif != NULL; netif = netif->next) {
        DEBUG_printf("Interface: %c%c%d\n", netif->name[0], netif->name[1], netif->num);
        DEBUG_printf("  IP Address: %s\n", ipaddr_ntoa(&netif->ip_addr));
        DEBUG_printf("  Netmask:    %s\n", ipaddr_ntoa(&netif->netmask));
        DEBUG_printf("  Gateway:    %s\n", ipaddr_ntoa(&netif->gw));
    }
    netif = &cyw43_state.netif[0]; // just use the first one
    
    netif_set_addr(netif, &ipaddr, &netmask, &gateway);
    netif_set_default(netif);
    netif_set_up(netif);

    // Start the dhcp server
    static dhcp_server_t dhcp_server;
    dhcp_server_init(&dhcp_server, &gateway, &netmask);
    uint8_t mac_address[6];
    cyw43_wifi_get_mac(&cyw43_state, CYW43_ITF_AP, mac_address);
    DEBUG_printf("AP Mode IP: %s\n", AP_IP_ADDR);
    DEBUG_printf("MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
           mac_address[0], mac_address[1], mac_address[2],
           mac_address[3], mac_address[4], mac_address[5]);
}

int main() {
    stdio_init_all();

    // Add metadata
    bi_decl(bi_program_description("Monitor voltage on ADC0."));
    bi_decl(bi_1pin_with_name(ADC_PIN, "ADC input"));
    bi_decl(bi_1pin_with_name(LED_PIN, "Status LED")); // see utils.h for pin definition
    bi_decl(bi_1pin_with_name(MPB_PIN, "Reset Button"));

    // Initialise ADC and GPIO pins
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_gpio_init(ADC_PIN); // Set GPIO26 (channel 0) high-impedance for analog input
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    mpButton.gpioSetup(MPB_PIN);
    mpButton.eventHandler(&buttonEvent); 
    gpio_set_irq_enabled_with_callback(MPB_PIN, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &extInterrupt);
    
    cyw43_arch_init_with_country(CYW43_COUNTRY_AUSTRALIA); // cyw43_arch_init();
    bool ap_mode = false;
    bool sta_mode_led = false;

    // Read Flash memory settings
    if (settings_init()) {
        cyw43_arch_enable_sta_mode();
        // Connect to the WiFI network - loop until connected
        onLED();
        while(cyw43_arch_wifi_connect_timeout_ms(
            get_settings()->wifi_ssid, 
            get_settings()->wifi_pwd, 
            CYW43_AUTH_WPA2_AES_PSK, 10000) != 0) {
                DEBUG_printf("WiFi connecting\n");
                if (reset_settings_flag) break; // button press - need to reset 
            }
        offLED();
    } else {
        // If no settings available boot into AP mode
       setup_ap_mode();
       ap_mode = true;
    }

    // Initialise web server, and MQTT client
    http_server_init();
    if (get_settings()->mqtt_pint > 0) {
        mqtt_client_init(); // only start mqtt if the pub interval non zero
    }
    absolute_time_t set_time = get_absolute_time();
    int publish_timer = get_settings()->mqtt_pint;

    while(true) {
        sys_check_timeouts();
        cyw43_arch_poll();

        if (to_ms_since_boot(get_absolute_time()) - to_ms_since_boot(set_time) > 1000) {
            if (publish_timer > 0) {
                if (publish_timer == 1){
                    publish_timer = get_settings()->mqtt_pint;  
                    mqtt_update();
                } else publish_timer -=1;
            }
            if (ap_mode) flashLED(2); // flash LED in AP mode
            else {
                if (sta_mode_led = !sta_mode_led) onLED(); else offLED(); // toggle LED in STA mode
                int status = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);
                if (status != CYW43_LINK_JOIN) {
                    DEBUG_printf("WiFi is disconnected.\n");
                    cyw43_arch_deinit();
                    watchdog_reboot(0, 0, 0); // start over
                }
            }
            set_time = get_absolute_time();
        }

        if (reset_settings_flag) {
            reset_settings_flag = false;
            reset_settings();
        }
        
    }
}