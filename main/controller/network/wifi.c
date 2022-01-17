#define _GNU_SOURCE /* To get defns of NI_MAXSERV and NI_MAXHOST */
#include <string.h>
#include <assert.h>
#include <time.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/mount.h>
#include <errno.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/if_link.h>

#include "utils/system_time.h"
#include "wifi.h"
#include "wpa_ctrl.h"
#include "log.h"

#ifdef TARGET_DEBUG
#define WPASOCK "/run/wpa_supplicant/wlp112s0"
#define remount_ro()
#define remount_rw()
#else
#define WPASOCK "/var/run/wpa_supplicant/wlan0"
#define ROOTFS  "/"

static inline void remount_ro() {
    if (mount("/dev/root", ROOTFS, "ext2", MS_REMOUNT | MS_RDONLY, NULL) < 0)
        log_error("Errore nel montare la partizione %s: %s (%i)", ROOTFS, strerror(errno), errno);
}

static inline void remount_rw() {
    if (mount("/dev/root", ROOTFS, "ext2", MS_REMOUNT, NULL) < 0)
        log_error("Errore nel montare la partizione %s: %s (%i)", ROOTFS, strerror(errno), errno);
}
#endif


static int strncpy_until(char *dest, size_t max, char *src, char until);


static struct wpa_ctrl *ctrl = NULL;


int wifi_init(void) {
    ctrl = wpa_ctrl_open(WPASOCK);

    if (ctrl == NULL) {
        log_warn("Non sono riuscito a collegarmi a wpa_supplicant");
        return -1;
    } else {
        log_info("Connesso con successo a wpa_supplicant");
        return 0;
    }
}

void wifi_connect(char *ssid, char *psk) {
    if (ctrl == NULL)
        return;

    char reply[128];
    char cmd[128];

    snprintf(cmd, 128, "SET_NETWORK 0 ssid \"%s\"", ssid);
    if (wpa_ctrl_request(ctrl, cmd, reply, 128, NULL) < 0)
        return;
    snprintf(cmd, 128, "SET_NETWORK 0 psk \"%s\"", psk);
    if (wpa_ctrl_request(ctrl, cmd, reply, 128, NULL) < 0)
        return;

    wpa_ctrl_request(ctrl, "DISCONNECT", reply, 128, NULL);
    wpa_ctrl_request(ctrl, "RECONNECT", reply, 128, NULL);
}

wifi_status_t wifi_status(char *ssid) {
    wifi_status_t res = WIFI_INACTIVE;
    char          reply[1024];
    char          value[32];
    if (ctrl == NULL) {
        return WIFI_INACTIVE;
    }

    if (wpa_ctrl_request(ctrl, "STATUS", reply, 1024, NULL) < 0) {
        return WIFI_INACTIVE;
    }

    char *newline = strtok(reply, "\n");

    while (newline && *newline != '\0') {
        if (sscanf(newline, "wpa_state=%s\n", value) == 1) {
            if (strcmp(value, "INACTIVE") == 0) {
                return WIFI_INACTIVE;
            } else if (strcmp(value, "DISCONNECTED") == 0) {
                return WIFI_INACTIVE;
            } else if (strcmp(value, "COMPLETED") == 0) {
                res = WIFI_CONNECTED;
            } else if (strcmp(value, "SCANNING") == 0) {
                return WIFI_SCANNING;
            } else {
                return WIFI_CONNECTING;
            }
        } else if (strncmp(newline, "ssid=", 5) == 0) {
            strncpy_until(ssid, MAX_NAME_SIZE, &newline[5], '\n');
        }
        newline = strtok(NULL, "\n");
    }
    return res;
}



void wifi_scan(void) {
    char reply[1024];
    if (ctrl == NULL) {
        return;
    }
    wpa_ctrl_request(ctrl, "SCAN", reply, 1024, NULL);
}


int wifi_read_scan(wifi_network_t **networks) {
    char *step;
    if (ctrl == NULL)
        return 0;

    char reply[1024];
    int  count = 0;

    wpa_ctrl_request(ctrl, "SCAN_RESULTS", reply, 1024, NULL);
    char *newline;     // La prima volta conta soltanto le linee con strchar
    newline = strchr(reply, '\n') + 1;
    newline = strchr(newline + 1, '\n');

    while (newline) {
        count++;
        newline = strchr(newline + 1, '\n');
    }

    if (*networks == NULL)
        *networks = malloc(sizeof(wifi_network_t) * count);
    else
        *networks = realloc(*networks, sizeof(wifi_network_t) * count);

    newline                     = strtok_r(reply, "\n", &step);
    wifi_network_t *ptr         = *networks;
    size_t          valid_count = 0;

    for (int i = 0; i < count; i++) {
        char *step2, *tab;
        newline = strtok_r(step, "\n", &step);

        if (!newline)
            break;

        if (!(tab = strtok_r(newline, "\t", &step2)))     // MAC Address
            continue;
        if (!(tab = strtok_r(step2, "\t", &step2)))     // Frequency
            continue;

        if (!(tab = strtok_r(step2, "\t", &step2)))     // signal strength
            continue;

        ptr[valid_count].signal = atoi(tab);

        if (!(tab = strtok_r(step2, "\t", &step2)))     // Flags
            continue;
        if (!(tab = strtok_r(step2, "\t", &step2)))     // SSID
            continue;

        strncpy_until(ptr[valid_count].ssid, MAX_NAME_SIZE, tab, '\n');
        valid_count++;
    }

    return valid_count;
}


int wifi_save_config(void) {
    if (ctrl == NULL) {
        return -1;
    }
    char reply[1024];
    remount_rw();
    int res = wpa_ctrl_request(ctrl, "SAVE_CONFIG", reply, 1024, NULL);
    remount_ro();
    return res;
}


int wifi_get_ip_address(char *ifname, char *ipaddr) {
    struct ifaddrs *ifaddr = NULL, *ifa = NULL;
    int             family, s, n;
    char            host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        log_warn("Non riesco a leggere le interfacce di rete: %s", strerror(errno));
        return 0;
    }

    /* Walk through linked list, maintaining head pointer so we
       can free list later */

    for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
        if (ifa->ifa_addr == NULL)
            continue;

        family = ifa->ifa_addr->sa_family;

        /* Display interface name and family (including symbolic
           form of the latter for the common families) */
        if (strcmp(ifa->ifa_name, ifname) == 0) {
            /* For an AF_INET* interface address, display the address */
            if (family == AF_INET) {
                s = getnameinfo(ifa->ifa_addr,
                                (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6), host,
                                NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
                if (s != 0) {
                    freeifaddrs(ifaddr);
                    return 0;
                }

                strcpy(ipaddr, host);
                freeifaddrs(ifaddr);
                return 1;
            }
        }
    }

    freeifaddrs(ifaddr);
    return 0;
}


int wifi_ntp_update(time_t *time) {

#define NTP_TIMESTAMP_DELTA 2208988800ULL

    int   sockfd;
    int   portno    = 123;                // NTP UDP port number.
    char *host_name = "pool.ntp.org";     // NTP server host-name.

    // Structure that defines the 48 byte NTP packet protocol.
    typedef struct {
        uint8_t li_vn_mode;     // Information bitfield
        uint8_t stratum;        // Eight bits. Stratum level of the local clock.
        uint8_t poll;           // Eight bits. Maximum interval between successive messages.
        uint8_t precision;      // Eight bits. Precision of the local clock.

        uint32_t rootDelay;          //  Total round trip delay time.
        uint32_t rootDispersion;     //  Max error aloud from primary clock
        uint32_t refId;              //  Reference clock identifier.

        uint32_t refTm_s;     //  Reference time-stamp seconds.
        uint32_t refTm_f;     //  Reference time-stamp fraction of a second.

        uint32_t origTm_s;     //  Originate time-stamp seconds.
        uint32_t origTm_f;     //  Originate time-stamp fraction of a second.

        uint32_t rxTm_s;     // Received time-stamp seconds.
        uint32_t rxTm_f;     // Received time-stamp fraction of a second.

        uint32_t txTm_s;     // Transmit time-stamp seconds.
        uint32_t txTm_f;     // Transmit time-stamp fraction of a second.

    } ntp_packet;     // Total: 384 bits or 48 bytes.

    ntp_packet packet = {0};

    // Set the first byte's bits to 00,011,011 for li = 0, vn = 3, and mode = 3.
    packet.li_vn_mode = 0x1b;

    // Create a UDP socket, convert the host-name to an IP address, set the port
    // number, connect to the server, send the packet, and then read in the
    // return packet.
    struct sockaddr_in serv_addr;     // Server address data structure.
    struct hostent    *server;        // Server data structure.

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        log_error("ERROR opening socket: %s", strerror(errno));
        return -1;
    }

    if ((server = gethostbyname(host_name)) == NULL) {
        log_error("ERROR, no such host: %s", strerror(errno));
        return -1;
    }

    // Zero out the server address structure.
    bzero((char *)&serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;

    // Copy the server's IP address to the server address structure.
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);

    // Convert the port number integer to network big-endian style and save it
    // to the server address structure.
    serv_addr.sin_port = htons(portno);

    // Call up the server using its IP address and port number.
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        log_error("ERROR connecting: %s", strerror(errno));
        return -1;
    }

    // Send it the NTP packet it wants. If n == -1, it failed.
    if (write(sockfd, (char *)&packet, sizeof(ntp_packet)) != sizeof(ntp_packet)) {
        log_error("ERROR writing to socket: %s", strerror(errno));
        return -1;
    }

    // Wait and receive the packet back from the server. If n == -1, it failed.
    if (read(sockfd, (char *)&packet, sizeof(ntp_packet)) != sizeof(ntp_packet)) {
        log_error("ERROR reading from socket: %s", strerror(errno));
        return -1;
    }

    // These two fields contain the time-stamp seconds as the packet left the
    // NTP server. The number of seconds correspond to the seconds passed since
    // 1900. ntohl() converts the bit/byte order from the network's to host's
    // "endianness".
    packet.txTm_s = ntohl(packet.txTm_s);     // Time-stamp seconds.
    packet.txTm_f = ntohl(packet.txTm_f);     // Time-stamp fraction of a second.

    // Extract the 32 bits that represent the time-stamp seconds (since NTP
    // epoch) from when the packet left the server. Subtract 70 years worth of
    // seconds from the seconds since 1900. This leaves the seconds since the
    // UNIX epoch of 1970.
    *time = (time_t)(packet.txTm_s - NTP_TIMESTAMP_DELTA);

    // Print the time we got from the server, accounting for local timezone and
    // conversion from UTC time.
    char *t          = ctime((const time_t *)time);
    t[strlen(t) - 1] = '\0';
    log_info("Data e ora scaricate tramite NTP: %s", t);
    return 0;
}

static void *wifi_time_sync_task(void *arg) {
    void (*cb)(time_t) = arg;
    time_t time;
    assert(wifi_ntp_update(&time) == 0);
    cb(time);
    pthread_exit(NULL);
}


pthread_t launch_wifi_time_sync_task(void (*cb)(time_t)) {
    pthread_t id;
    pthread_create(&id, NULL, wifi_time_sync_task, (void *)cb);
    return id;
}


static int strncpy_until(char *dest, size_t max, char *src, char until) {
    assert(src);
    assert(dest);

    int count = 0;
    while (*src != '\0' && *src != until && max > 0) {
        *dest++ = *src++;
        count++;
        max--;
    }
    *dest = '\0';
    return count;
}