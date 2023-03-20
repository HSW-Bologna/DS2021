#ifndef APP_CONF_H_INCLUDED
#define APP_CONF_H_INCLUDED


#define DISPLAY_HORIZONTAL_RESOLUTION 1024
#define DISPLAY_VERTICAL_RESOLUTION   600


#define SOFTWARE_VERSION    "0.A.0"
#define SOFTWARE_BUILD_DATE __DATE__


#ifdef TARGET_DEBUG
#define DEFAULT_BASE_PATH "./data"
#define IFWIFI            "wlp112s0"
#define IFETH             "enp109s0"
#define HTTP_SERVER_PORT  8080
#define CONFIG_LOG_LEVEL  LOG_DEBUG
#else
#define DEFAULT_BASE_PATH "/mnt/data"
#define IFWIFI            "wlan0"
#define IFETH             "eth0"
#define HTTP_SERVER_PORT  80
#define CONFIG_LOG_LEVEL  LOG_INFO
#endif

#define CONFIG_DATA_VERSION 1

#define DRIVE_MOUNT_PATH               "/tmp/mnt"
#define INDEX_FILE_NAME                "index.txt"
#define DEFAULT_PARAMS_PATH            DEFAULT_BASE_PATH "/parametri"
#define DEFAULT_PROGRAMS_PATH          DEFAULT_BASE_PATH "/programmi"
#define DEFAULT_PATH_FILE_DATA_VERSION DEFAULT_BASE_PATH "version.txt"
#define DEFAULT_PATH_FILE_PARMAC       DEFAULT_PARAMS_PATH "/parmac.bin"
#define DEFAULT_PATH_FILE_PASSWORD     DEFAULT_PARAMS_PATH "/password.txt"
#define DEFAULT_PATH_FILE_INDEX        DEFAULT_PROGRAMS_PATH "/" INDEX_FILE_NAME
#define LOGFILE                        "/tmp/DS2021_log.txt"
#define MAX_LOGFILE_SIZE               4000000UL
#define SKELETON_KEY                   "5510726719"
#define SETTINGS_PASSWORD              "72346"
#define LANGUAGE_RESET_DELAY           15000UL
#define ARCHIVE_EXTENSION              ".DS2021.tar.gz"


#endif