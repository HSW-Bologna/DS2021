#ifndef WIFI_H_INCLUDED
#define WIFI_H_INCLUDED

#include "model/model.h"

int       wifi_init(void);
int       wifi_get_ip_address(char *ifname, char *ipaddr);
int       wifi_read_scan(wifi_network_t **networks);
int       wifi_save_config(void);
int       wifi_ntp_update(time_t *time);
pthread_t launch_wifi_time_sync_task(void (*cb)(time_t));

wifi_status_t wifi_status(char *ssid);
void          wifi_connect(char *ssid, char *psk);
void          wifi_scan(void);

#endif