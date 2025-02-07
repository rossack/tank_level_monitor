/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * LWIP Documentation: http://www.nongnu.org/lwip/
 * https://www.raspberrypi.com/documentation/pico-sdk/
 *
 */
#include <stdio.h>
#include <time.h>

#include "hardware/structs/rosc.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/watchdog.h"
#include "hardware/timer.h"

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/dns.h"
#include "lwip/netif.h"
#include "lwip/ip_addr.h"

#include "lwip/altcp_tcp.h"
#include "lwip/altcp_tls.h"
#include "lwip/apps/mqtt.h"
#include "lwip/apps/mqtt_priv.h"

#include "tiny-json.h"
#include "mqtt_client.h"
#include "utils.h"
#include "setting_defaults.h"
#include "settings.h"
#include "debug.h"

#define MAX_INTERVAL 3600 // maximum number of seconds between mqtt pub's

typedef struct MQTT_CLIENT_T_ {
    ip_addr_t remote_addr;
    mqtt_client_t *mqtt_client;
    u32_t counter;
    u32_t reconnect;
} MQTT_CLIENT_T;


//
// Globals
//
MQTT_CLIENT_T * mqtt_client_state;
u8_t buffer[1025]; // For incoming messages - can arrive across multiple callbacks
u32_t data_in = 0;
u8_t data_len = 0;
//
// End Globals
//


u32_t get_mqtt_json(char *sbuf, uint sz) {
    snprintf(sbuf, sz, "{\"ID\": \"%s\",\"Host\": \"%s\",\"Port\": %u,\"User\": \"%s\",\"PubCount\": %u,\"PubInt\": %u}",
      get_settings()->mqtt_cid,
      get_settings()->mqtt_host,
      get_settings()->mqtt_port,
      get_settings()->mqtt_user,
      mqtt_client_state->counter,
      get_settings()->mqtt_pint
      );
    return strlen(sbuf);
}

void dns_found(const char *name, const ip_addr_t *ipaddr, void *callback_arg) {
    MQTT_CLIENT_T *state = (MQTT_CLIENT_T*)callback_arg;
    DEBUG_printf("DNS query resolved to: %s.\n", ip4addr_ntoa(ipaddr));
    state->remote_addr = *ipaddr;
}

void dns_lookup(MQTT_CLIENT_T *state) {
    cyw43_arch_lwip_begin();
    err_t err = dns_gethostbyname(get_settings()->mqtt_host, &(state->remote_addr), dns_found, state);
    cyw43_arch_lwip_end();

    if (err == ERR_ARG) {
        DEBUG_printf("DNS query failed\n");
        return;
    }

    if (err == ERR_OK) {
        DEBUG_printf("No lookup needed\n");
        return;
    }

    while (state->remote_addr.addr == 0) {
        cyw43_arch_poll();
        sleep_ms(100);
    }
}


static void mqtt_pub_start_cb(void *arg, const char *topic, u32_t tot_len) {
    DEBUG_printf("mqtt_pub_start_cb: topic %s\n", topic);

    if (tot_len > 1024) {
        DEBUG_printf("Message too big\n");
    } else {
        data_in = tot_len;
        data_len = 0;
    }
}



static void mqtt_pub_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags) {
    if (data_in > 0) {
        data_in -= len;
        memcpy(&buffer[data_len], data, len);
        data_len += len;

        if (data_in == 0) {
            buffer[data_len] = 0;
            DEBUG_printf("Message received: %s\n", &buffer);
            json_t pool[ 4 ]; // how many json properties
            u8_t * str = buffer; //str - gets modified in the create call
            json_t const* parent = json_create( (char *)str, pool, 4 );
            if ( parent != NULL ) {
                json_t const* pintField = json_getProperty( parent, "PInt" );
                if (pintField != NULL) {
                    uint i = json_getInteger(pintField);
                    if ((i > 0) && (i < MAX_INTERVAL)) {
                        // set_mqtt_pint(i);
                        // update and save settings TBD
                    }
                }
            }
        }
    }
}

static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status) {
    if (status != 0) {
        DEBUG_printf("Error during connection: err %d.\n", status);
    } else {
        DEBUG_printf("MQTT connected.\n");
    }
}

void mqtt_pub_request_cb(void *arg, err_t err) {
    MQTT_CLIENT_T *state = (MQTT_CLIENT_T *)arg;
    state->counter++;
    DEBUG_printf("Published %d\n", state->counter);
}

void mqtt_sub_request_cb(void *arg, err_t err) {
    DEBUG_printf("mqtt_sub_request_cb: err %d\n", err);
}

err_t mqtt_do_publish(MQTT_CLIENT_T *state)
{
  char pubBuffer[256];
  err_t err;
  u8_t qos = 0; /* 0 1 or 2, see MQTT specification.  AWS IoT does not support QoS 2 */
  u8_t retain = 0;

  get_status_json(pubBuffer, sizeof(pubBuffer));
  cyw43_arch_lwip_begin();
  err = mqtt_publish(state->mqtt_client, get_settings()->mqtt_state_topic, pubBuffer, strlen(pubBuffer), qos, retain, mqtt_pub_request_cb, state);
  cyw43_arch_lwip_end();
  get_sensor_json(pubBuffer, sizeof(pubBuffer));
  cyw43_arch_lwip_begin();
  err = mqtt_publish(state->mqtt_client, get_settings()->mqtt_sensor_topic, pubBuffer, strlen(pubBuffer), qos, retain, mqtt_pub_request_cb, state);
  cyw43_arch_lwip_end();

  return err;
}

err_t mqtt_do_connect(MQTT_CLIENT_T *state) {
    struct mqtt_connect_client_info_t ci;
    err_t err;
    memset(&ci, 0, sizeof(ci));
    ci.client_id = get_settings()->mqtt_cid;
    ci.client_user = get_settings()->mqtt_user;
    ci.client_pass = get_settings()->mqtt_pwd;
    ci.keep_alive = 0;
    ci.will_topic = NULL;
    ci.will_msg = NULL;
    ci.will_retain = 0;
    ci.will_qos = 0;

    const struct mqtt_connect_client_info_t *client_info = &ci;
    err = mqtt_client_connect(state->mqtt_client,
                            &(state->remote_addr),
                            get_settings()->mqtt_port,
                            mqtt_connection_cb,
                            state, client_info);


    DEBUG_printf("mqtt_connect: %d\n", err);
    return err;
}


err_t mqtt_update() {
    err_t err;
    if (mqtt_client_is_connected(mqtt_client_state->mqtt_client)) {
        if ((err = mqtt_do_publish(mqtt_client_state)) == ERR_OK) {
            //flashLED(1);
        }
    }
    return err;
}


err_t mqtt_client_init() {
    err_t err;
    mqtt_client_state = calloc(1, sizeof(MQTT_CLIENT_T));
    if (!mqtt_client_state) return ERR_ABRT;
    mqtt_client_state->counter = 0;

    dns_lookup(mqtt_client_state);
    mqtt_client_state->mqtt_client = mqtt_client_new();
    err = mqtt_do_connect(mqtt_client_state);
    if (err != ERR_OK ) {
        DEBUG_printf("Failed to connect to MQTT Broker\n");
        return err;
    }

    mqtt_sub_unsub(mqtt_client_state->mqtt_client, get_settings()->mqtt_config_topic, 0, mqtt_sub_request_cb, 0, 1); // subscribe to CONFIG topic
    mqtt_set_inpub_callback(mqtt_client_state->mqtt_client, mqtt_pub_start_cb, mqtt_pub_data_cb, 0);
    return err;
}
