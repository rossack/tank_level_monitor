
#include <stdio.h>
#include <stdarg.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcpip.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/dhcp.h"
#include "setting_defaults.h"
#include "dhcpserver/dhcpserver.h"
#include "settings.h"
#include "debug.h"
#include "wifi.h"



WifiConn::WifiConn() {

}

void WifiConn::init(uint country){
    cyw43_arch_init_with_country(CYW43_COUNTRY_AUSTRALIA);

}
void WifiConn::deinit() {
    cyw43_arch_deinit();
}


void WifiConn::start(Mode m, volatile bool *bailout) {
    mode = m;
    if (m == WIFI_STA) {
        cyw43_arch_enable_sta_mode();
        uint32_t pm;
        cyw43_wifi_get_pm(&cyw43_state, &pm);
        DEBUG_printf("PM: %x",pm); //a11142
        cyw43_wifi_pm(&cyw43_state, 0xa11140); // CYW43_NO_POWERSAVE
        // Connect to the WiFI network - loop until connected or giveup flag gets set
        while(cyw43_arch_wifi_connect_timeout_ms(
            get_settings()->wifi_ssid,
            get_settings()->wifi_pwd,
            CYW43_AUTH_WPA2_AES_PSK, 5000) != 0) {
                DEBUG_printf("WiFi connecting\n");
                if (bailout) break; // check for button press
            }
        connected = true;
    } else if (m == WIFI_AP) {
        // wifi_scan();
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
        dhcp_server_init(&dhcp_server, &gateway, &netmask);
        uint8_t mac_address[6];
        cyw43_wifi_get_mac(&cyw43_state, CYW43_ITF_AP, mac_address);
        DEBUG_printf("AP Mode IP: %s\n", AP_IP_ADDR);
        DEBUG_printf("MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
           mac_address[0], mac_address[1], mac_address[2],
           mac_address[3], mac_address[4], mac_address[5]);
        connected = true;
    } else {
        DEBUG_printf("Invalid WiFi Mode\n");
    }
}

Mode WifiConn::getMode() {
    return mode;
}

bool WifiConn::isConnected() {
    int status;
    if (mode == WIFI_AP) {
        status = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_AP);
    } else {
        status = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);
    }
    if (status == CYW43_LINK_JOIN) {
        // Connected to wifi
        return true;
    } else {
        DEBUG_printf("WiFi Link Status: %u.\n",status);
        connected = false;
        return false;
    }
}

int scan_result_cb(void * parm, const cyw43_ev_scan_result_t * scan_result) {

    DEBUG_printf("%s: SSID: %s | Signal Strength: %d dBm | Channel: %d\n", parm,
            scan_result->ssid, scan_result->rssi, scan_result->channel);
    return 0;
}

int wifi_scan() {
    cyw43_arch_enable_sta_mode();

    // Start the Wi-Fi scan
    char hello[10] = "Hello";
    cyw43_wifi_scan_options_t scan_options = {0};
    cyw43_wifi_scan(&cyw43_state, &scan_options, &hello, &scan_result_cb);

    // Wait for the scan to complete
    while (cyw43_wifi_scan_active(&cyw43_state)) {
        cyw43_arch_poll();
        sleep_ms(100);
    }
    cyw43_arch_disable_sta_mode();
    return 0;
}


