#include "lwip/apps/httpd.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwipopts.h"
#include "ssi.h"
#include "cgi.h"
#include "lwip/ip4_addr.h"

// WIFI Credentials - take care if pushing to github!
const char WIFI_SSID[] = "Benjamin's iPhone";
const char WIFI_PASSWORD[] = "benjaminiphone67";

int main()
{
    stdio_init_all();

    cyw43_arch_init();

    cyw43_arch_enable_sta_mode();

    // Connect to the WiFI network - loop until connected
    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000) != 0)
    {
        printf("Attempting to connect...\n");
    }
    // Print a success message once connected
    printf("Connected! \n");
    ip4_addr_t ipaddr, netmask, gw;
    struct netif *netif = netif_default;
    if (netif)
    {
        printf("IP Address: %s\n", ip4addr_ntoa(&netif->ip_addr));
        printf("Netmask: %s\n", ip4addr_ntoa(&netif->netmask));
        printf("Gateway: %s\n", ip4addr_ntoa(&netif->gw));
    }
    else
    {
        printf("Network interface not found\n");
    }

    // Initialise web server
    httpd_init();
    printf("Http server initialised\n");

    // Configure SSI and CGI handler
    ssi_init();
    printf("SSI Handler initialised\n");
    cgi_init();
    printf("CGI Handler initialised\n");

    // Infinite loop
    while (1)
        ;
}