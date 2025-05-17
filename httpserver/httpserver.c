#include <stdio.h>
#include <stdarg.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/adc.h"
#include "lwipopts.h"
#include "lwip/tcp.h"
#include "lwip/ip_addr.h"
#include "lwip/inet.h"
#include "lwip/apps/httpd.h"
#include "mqtt_client.h"
#include "setting_defaults.h"
#include "settings.h"
#include "utils.h"
#include "debug.h"

#define MAX_POST_BUFFER_SIZE 1024
#define MAX_PARAM_NAME_LEN 50
#define MAX_PARAM_VALUE_LEN 100
static char post_data[MAX_POST_BUFFER_SIZE];
static int post_data_len;

extern uint get_wifi_json(char *sbuf, uint sz);
extern uint get_data_json(char *sbuf, uint sz);

void process_key_value(char *key, char *value) {

  if (strlen(value) == 0) return;
  if (strcmp(key, "ssid") == 0) {
    set_wifi_ssid(value);
  } else if (strcmp(key, "pwd") == 0) {
    set_wifi_pwd(value);
  } else if (strcmp(key, "dataLogInt") == 0) {
    set_data_lint(atoi(value));
  } else if (strcmp(key, "mqHost") == 0) {
    set_mqtt_host(value);
  } else if (strcmp(key, "mqPort") == 0) {
    set_mqtt_port(atoi(value));
  } else if (strcmp(key, "mqUser") == 0) {
    set_mqtt_user(value);
  } else if (strcmp(key, "mqPwd") == 0) {
    set_mqtt_pwd(value);
  } else if (strcmp(key, "mqPubInt") == 0) {
    set_mqtt_pint(atoi(value));
  } else if (strcmp(key, "mqStateTopic") == 0) {
    set_mqtt_state_topic(value);
  } else if (strcmp(key, "mqSensTopic") == 0) {
    set_mqtt_sensor_topic(value);
  } else if (strcmp(key, "mqConfTopic") == 0) {
    set_mqtt_config_topic(value);
  } else {
    DEBUG_printf("Unknown key: %s\n",key);
  }
}



/*
** Decode URL-encoded data
** (like %20 for spaces and + for spaces in form data) into a readable string.
*/
void url_decode(char *dst, const char *src) {
    char a, b;
    while (*src) {
        if ((*src == '%') && ((a = src[1]) && (b = src[2])) && (isxdigit(a) && isxdigit(b))) {
            if (a >= 'a') a -= 'a' - 'A';
            if (a >= 'A') a -= ('A' - 10);
            else a -= '0';
            if (b >= 'a') b -= 'a' - 'A';
            if (b >= 'A') b -= ('A' - 10);
            else b -= '0';
            *dst++ = 16 * a + b;
            src += 3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}


/*
** This function takes the raw POST data (URL-encoded key-value pairs)
** and splits it into individual key-value pairs using strtok().
** Each key and value is then URL-decoded using the url_decode() function.
**
*/
void parse_post_data(char *data) {
    char *key, *value;
    char param_name[MAX_PARAM_NAME_LEN];
    char param_value[MAX_PARAM_VALUE_LEN];

    // Tokenize the POST data by '&' (key-value pairs)
    key = strtok(data, "&");
    while (key != NULL) {
        // Split the key-value pair by '='
        value = strchr(key, '=');
        if (value != NULL) {
            *value = '\0';  // Terminate the key string
            value++;        // Move to the value part

            // Decode the key and value (they may be URL-encoded)
            url_decode(param_name, key);
            url_decode(param_value, value);

            // Process the parameters
            process_key_value(param_name, param_value);
            DEBUG_printf("Key: %s = %s\n", param_name, param_value);
        }
        // Get the next key-value pair
        key = strtok(NULL, "&");
    }
    //
    // If settings have been updated, save them to flash memory
    //
    save_settings();
}


err_t httpd_post_begin(void *connection, const char *uri, const char *http_request,
                       u16_t http_request_len, int content_len, char *response_uri,
                       u16_t response_uri_len, u8_t *post_auto_wnd) {
    DEBUG_printf("POST request URI: %s, Content-Length: %u\n", uri, content_len);

    // Initialize the buffer for POST data
    memset(post_data, 0, sizeof(post_data));
    post_data_len = 0;

    // Check if the content length exceeds the buffer size
    if (content_len > MAX_POST_BUFFER_SIZE - 1) {
        return ERR_MEM;  // Not enough space to handle this request
    }

    return ERR_OK;
}


err_t httpd_post_receive_data(void *connection, struct pbuf *p) {
    size_t len = p->tot_len;

    if (post_data_len + len > MAX_POST_BUFFER_SIZE - 1) {
        // Data too large
        return ERR_MEM;
    }

    // Copy POST data to buffer
    pbuf_copy_partial(p, post_data + post_data_len, len, 0);
    post_data_len += len;

    //DEBUG_printf("Received POST data: %s\n", post_data);
    // Free the pbuf
    pbuf_free(p);

    return ERR_OK;
}

void httpd_post_finished(void *connection, char *response_uri, u16_t response_uri_len) {

    parse_post_data(post_data);
    // Set the response URI (e.g., redirect to a success page)
    // TBD: If factory reset needs a different response
    //
    snprintf(response_uri, response_uri_len, "/index.shtml");
}


// CGI handler which is run when a request for /led.cgi is detected
// <a href="/led.cgi?led=1"><button>LED ON</button></a>
// <a href="/led.cgi?led=0"><button>LED OFF</button></a>
const char * cgi_led_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    // Check if an request for LED has been made (/led.cgi?led=x)
    if (strcmp(pcParam[0] , "led") == 0){
        // Look at the argument to check if LED is to be turned on (x=1) or off (x=0)
        if(strcmp(pcValue[0], "0") == 0)
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        else if(strcmp(pcValue[0], "1") == 0)
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    }

    // Send the index page back to the user
    return "/index.shtml";
}


// tCGI Struct
// Fill this with all of the CGI requests and their respective handlers
static const tCGI cgi_handlers[] = {
    {"/led.cgi", cgi_led_handler}
};


// SSI tags - tag length limited to 8 bytes by default
const char * ssi_tags[] = {"config",
                          "data",
                          "sensor",
                          "status",
                          "mqtt",
                          "wifi",
                          "reset"};


/*
#define SSI_LARGE_DATA_SIZE 1024
static char ssi_large_data_buffer[SSI_LARGE_DATA_SIZE];
static u16_t ssi_large_data_size = 0;
static u16_t ssi_large_data_index = 0;
*/
u16_t ssi_handler(int iIndex, char *pcInsert, int iInsertLen) {

  // The default maximum size of the inserted string is 192 characters
  // The config and data JSON objects are bigger so increase
  // LWIP_HTTPD_MAX_TAG_INSERT_LEN in lwiopts.h to 1024
  //
  size_t inserted;
  switch (iIndex) {
  case 0: // config
    {
      inserted = get_config_json(pcInsert, iInsertLen);
    }
    break;
  case 1: // data
    {
      //ssi_large_data_size = get_data_json(ssi_large_data_buffer, sizeof(ssi_large_data_buffer));
      inserted = get_data_json(pcInsert, iInsertLen);
    }
    break;
  case 2: // sensor
    {
      inserted = get_sensor_json(pcInsert, iInsertLen);
    }
    break;
  case 3: // status
    {
      inserted = get_status_json(pcInsert, iInsertLen);
    }
    break;
  case 4: // mqtt
    {
      inserted = get_mqtt_json(pcInsert, iInsertLen);
    }
    break;
    case 5: // wifi scan
    {
      inserted = get_wifi_json(pcInsert, iInsertLen);
    }
    break;
    case 6: // do a factory reset
    {
      char frStr[]  = "{\"Reset\" : \"OK\"}";
      memcpy(pcInsert,frStr, strlen(frStr));
      inserted = strlen(frStr);
      reset_settings();
    }
    break;
  default:
    inserted = 0;
    break;
  }
  return (u16_t)inserted;
}


void http_server_init() {

    httpd_init();
    http_set_ssi_handler(ssi_handler, ssi_tags, 7); // LWIP_ARRAYSIZE(ssi_tags)
    http_set_cgi_handlers(cgi_handlers, 1);  // specify number of handlers
}



