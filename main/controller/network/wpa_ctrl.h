
#ifndef UWIFI_WPA_CTRL_H_
#define UWIFI_WPA_CTRL_H_

/*
 * wpa_supplicant and hostapd control interface
 *
 * Taken from the hostapd source code and simplified
 */

#ifdef __cplusplus
extern "C" {
#endif

struct wpa_ctrl* wpa_ctrl_open(const char* ctrl_path);

void wpa_ctrl_close(struct wpa_ctrl* ctrl);

int wpa_ctrl_request(struct wpa_ctrl *ctrl, const char *cmd,
		     char *reply, size_t reply_len,
		     void (*msg_cb)(char *msg, size_t len));

#ifdef __cplusplus
}
#endif

#endif