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
#include "httpserver/httpserver.h"
#include "button.h"
#include "setting_defaults.h"
#include "settings.h"
#include "flash_memory.h"
#include "utils.h"
#include "wifi.h"
#include "debug.h"

#define MPB_PIN 13 // GPIO pin to Momentary Push Button
#define ADC_PIN 26 // ADC0 GPIO pin

MPButton mpButton; // Momentary Push button
WifiConn wifi; // WiFi connection
volatile bool bailout = false;

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
            bailout= true;
            break;
    }
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

    // wifi.init();
    cyw43_arch_init_with_country(CYW43_COUNTRY_AUSTRALIA); // cyw43_arch_init()
    bool status_led = false;

    // Read Flash memory settings
    if (settings_init()) {
        onLED();
        wifi.start(WIFI_STA, &bailout);
        offLED();
    } else {
        // If no settings available boot into AP mode
        wifi.start(WIFI_AP, &bailout);
    }

    // Initialise web server, and MQTT client
    http_server_init();
    if (get_settings()->mqtt_pint > 0) {
        mqtt_client_init(); // only start mqtt if the pub interval non zero
    }
    absolute_time_t set_time = get_absolute_time();
    int publish_timer = get_settings()->mqtt_pint;
    watchdog_enable(2000, true);
    //
    // Idle loop
    //
    while(true) {
        sys_check_timeouts();
        cyw43_arch_poll();
        watchdog_update();

        if (to_ms_since_boot(get_absolute_time()) - to_ms_since_boot(set_time) > 1000) {
            if (publish_timer > 0) {
                if (publish_timer == 1){
                    publish_timer = get_settings()->mqtt_pint;
                    mqtt_update();
                } else publish_timer -=1;
            }
            if (wifi.getMode() == WIFI_AP) flashLED(2); // flash LED in AP mode
            else {
                if (status_led = !status_led) onLED(); else offLED(); // toggle LED in WIFI_STA mode
                if (!wifi.isConnected()) {
                    wifi.deinit();
                    watchdog_reboot(0, 0, 0); // start over
                }
            }
            set_time = get_absolute_time();
        }

        if (bailout) {
            // user wants to reset with a new configuration
            reset_settings();
        }

    }
}