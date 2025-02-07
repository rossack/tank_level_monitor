/*
**
** Class to handle a WiFi connection either as an Access Point (AP) or Station mode (STA)
**
*/
#include <stdio.h>
#include "dhcpserver/dhcpserver.h"

enum Mode {
    WIFI_NONE = 0,
    WIFI_STA = 1,
    WIFI_AP = 2
};

class WifiConn
{
    private:
    Mode mode = WIFI_NONE; // Type of connection
    struct netif *netif;
    dhcp_server_t dhcp_server;
    char * scan_result;

    public:
    WifiConn();
    void init(uint country);
    void deinit();
    void start(Mode mode, volatile bool *bailout);
    void stop();
    Mode getMode();
    bool isConnected();
    int scan(char *sbuf, uint sz);

};
