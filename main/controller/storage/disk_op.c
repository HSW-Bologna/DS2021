#include <poll.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include "utils/socketq.h"
#include "disk_op.h"
#include "storage.h"
#include "config/app_conf.h"
#include "../network/wifi.h"
#include "log.h"


#define REQUEST_SOCKET_PATH  "/tmp/.application_disk_request_socket"
#define RESPONSE_SOCKET_PATH "/tmp/.application_disk_response_socket"
#define MOUNT_ATTEMPTS       5
#define APP_UPDATE           "/tmp/mnt/DS2021.bin"

#ifdef TARGET_DEBUG
#define TEMPORARY_APP "./newapp"
#else
#define TEMPORARY_APP "/root/newapp"
#endif


typedef struct {
    name_t *names;
    size_t  num;
} disk_op_name_list_t;


typedef enum {
    DISK_OP_MESSAGE_CODE_LOAD_PARMAC,
    DISK_OP_MESSAGE_CODE_LOAD_PROGRAMS,
    DISK_OP_MESSAGE_CODE_SAVE_PARMAC,
    DISK_OP_MESSAGE_CODE_SAVE_PROGRAM_INDEX,
    DISK_OP_MESSAGE_CODE_SAVE_PROGRAM,
    DISK_OP_MESSAGE_CODE_SAVE_PASSWORD,
    DISK_OP_MESSAGE_CODE_REMOVE_PROGRAM,
    DISK_OP_MESSAGE_CODE_SAVE_WIFI_CONFIG,
    DISK_OP_MESSAGE_CODE_READ_FILE,
    DISK_OP_MESSAGE_CODE_EXPORT_CURRENT_MACHINE,
    DISK_OP_MESSAGE_CODE_IMPORT_CURRENT_MACHINE,
    DISK_OP_MESSAGE_CODE_FIRMWARE_UPDATE,
} disk_op_message_code_t;


typedef struct {
    disk_op_message_code_t   code;
    disk_op_callback_t       callback;
    disk_op_error_callback_t error_callback;
    void                    *data;
    void                    *arg;
} disk_op_message_t;


static void *disk_interaction_task(void *args);
static void  simple_request(int code, disk_op_callback_t cb, disk_op_error_callback_t errcb, void *arg);


static socketq_t       requestq;
static socketq_t       responseq;
static pthread_mutex_t sem;
static int             drive_mounted      = 0;
static size_t          drive_machines_num = 0;
static name_t         *drive_machines     = NULL;


void disk_op_init(void) {
    int res1 = socketq_init(&requestq, REQUEST_SOCKET_PATH, sizeof(disk_op_message_t));
    int res2 = socketq_init(&responseq, RESPONSE_SOCKET_PATH, sizeof(disk_op_response_t));
    assert(res1 == 0 && res2 == 0);

    assert(pthread_mutex_init(&sem, NULL) == 0);

    pthread_t id;
    pthread_create(&id, NULL, disk_interaction_task, NULL);
    pthread_detach(id);
}


void disk_op_remove_program(char *filename, disk_op_callback_t cb, disk_op_error_callback_t errcb, void *arg) {
    char *name = malloc(strlen(filename) + 1);
    assert(name != NULL);
    strcpy(name, filename);
    disk_op_message_t msg = {
        .code           = DISK_OP_MESSAGE_CODE_REMOVE_PROGRAM,
        .data           = name,
        .callback       = cb,
        .error_callback = errcb,
        .arg            = arg,
    };
    socketq_send(&requestq, (uint8_t *)&msg);
}


void disk_op_save_password(char *password, disk_op_callback_t cb, disk_op_error_callback_t errcb, void *arg) {
    char *password_copy = malloc(strlen(password) + 1);
    assert(password_copy != NULL);
    strcpy(password_copy, password);
    disk_op_message_t msg = {
        .code           = DISK_OP_MESSAGE_CODE_SAVE_PASSWORD,
        .data           = password_copy,
        .callback       = cb,
        .error_callback = errcb,
        .arg            = arg,
    };
    socketq_send(&requestq, (uint8_t *)&msg);
}


void disk_op_save_program(dryer_program_t *p, disk_op_callback_t cb, disk_op_error_callback_t errcb, void *arg) {
    name_t *program_copy = malloc(sizeof(dryer_program_t));
    memcpy(program_copy, p, sizeof(dryer_program_t));
    assert(program_copy != NULL);
    disk_op_message_t msg = {
        .code           = DISK_OP_MESSAGE_CODE_SAVE_PROGRAM,
        .data           = program_copy,
        .callback       = cb,
        .error_callback = errcb,
        .arg            = arg,
    };
    socketq_send(&requestq, (uint8_t *)&msg);
}


void disk_op_save_program_index(model_t *pmodel, disk_op_callback_t cb, disk_op_error_callback_t errcb, void *arg) {
    disk_op_name_list_t *list  = (malloc(sizeof(disk_op_name_list_t)));
    name_t              *names = malloc(sizeof(name_t) * model_get_num_programs(pmodel));
    assert(names != NULL);
    for (size_t i = 0; i < model_get_num_programs(pmodel); i++) {
        memcpy(names[i], model_get_program(pmodel, i)->filename, sizeof(name_t));
    }
    list->names           = names;
    list->num             = model_get_num_programs(pmodel);
    disk_op_message_t msg = {
        .code           = DISK_OP_MESSAGE_CODE_SAVE_PROGRAM_INDEX,
        .data           = list,
        .callback       = cb,
        .error_callback = errcb,
        .arg            = arg,
    };
    socketq_send(&requestq, (uint8_t *)&msg);
}


void disk_op_save_parmac(parmac_t *parmac, disk_op_callback_t cb, disk_op_error_callback_t errcb, void *arg) {
    parmac_t *parmac_copy = malloc(sizeof(parmac_t));
    assert(parmac_copy != NULL);
    memcpy(parmac_copy, parmac, sizeof(parmac_t));
    disk_op_message_t msg = {
        .code           = DISK_OP_MESSAGE_CODE_SAVE_PARMAC,
        .data           = parmac_copy,
        .callback       = cb,
        .error_callback = errcb,
        .arg            = arg,
    };
    socketq_send(&requestq, (uint8_t *)&msg);
}


void disk_op_export_current_machine(char *name, disk_op_callback_t cb, disk_op_error_callback_t errcb, void *arg) {
    char *name_copy = malloc(strlen(name) + 1);
    assert(name_copy != NULL);
    strcpy(name_copy, name);
    disk_op_message_t msg = {
        .code           = DISK_OP_MESSAGE_CODE_EXPORT_CURRENT_MACHINE,
        .data           = name_copy,
        .callback       = cb,
        .error_callback = errcb,
        .arg            = arg,
    };
    socketq_send(&requestq, (uint8_t *)&msg);
}


void disk_op_import_current_machine(char *name, disk_op_callback_t cb, disk_op_error_callback_t errcb, void *arg) {
    char *name_copy = malloc(strlen(name) + 1);
    assert(name_copy != NULL);
    strcpy(name_copy, name);
    disk_op_message_t msg = {
        .code           = DISK_OP_MESSAGE_CODE_IMPORT_CURRENT_MACHINE,
        .data           = name_copy,
        .callback       = cb,
        .error_callback = errcb,
        .arg            = arg,
    };
    socketq_send(&requestq, (uint8_t *)&msg);
}



void disk_op_load_programs(disk_op_callback_t cb, disk_op_error_callback_t errcb, void *arg) {
    simple_request(DISK_OP_MESSAGE_CODE_LOAD_PROGRAMS, cb, errcb, arg);
}


void disk_op_load_parmac(disk_op_callback_t cb, disk_op_error_callback_t errcb, void *arg) {
    simple_request(DISK_OP_MESSAGE_CODE_LOAD_PARMAC, cb, errcb, arg);
}


void disk_op_save_wifi_config(disk_op_callback_t cb, disk_op_error_callback_t errcb, void *arg) {
    simple_request(DISK_OP_MESSAGE_CODE_SAVE_WIFI_CONFIG, cb, errcb, arg);
}


void disk_op_firmware_update(disk_op_callback_t cb, disk_op_error_callback_t errcb, void *arg) {
    simple_request(DISK_OP_MESSAGE_CODE_FIRMWARE_UPDATE, cb, errcb, arg);
}


void disk_op_read_file(char *name, disk_op_callback_t cb, disk_op_error_callback_t errcb, void *arg) {
    size_t len       = strlen(name) + 1;
    char  *file_name = malloc(len);
    memset(file_name, 0, len);
    strcpy(file_name, name);
    assert(file_name != NULL);
    disk_op_message_t msg = {
        .code           = DISK_OP_MESSAGE_CODE_READ_FILE,
        .data           = file_name,
        .callback       = cb,
        .error_callback = errcb,
        .arg            = arg,
    };
    socketq_send(&requestq, (uint8_t *)&msg);
}


int disk_op_is_drive_mounted(void) {
    pthread_mutex_lock(&sem);
    int res = drive_mounted;
    pthread_mutex_unlock(&sem);
    return res;
}


size_t disk_op_drive_machines(name_t **machines) {
    if (!disk_op_is_drive_mounted()) {
        return 0;
    }

    pthread_mutex_lock(&sem);
    size_t res = drive_machines_num;
    *machines  = realloc(*machines, sizeof(name_t) * drive_machines_num);
    memcpy(*machines, drive_machines, sizeof(name_t) * drive_machines_num);
    pthread_mutex_unlock(&sem);

    return res;
}


int disk_op_manage_response(model_t *pmodel) {
    disk_op_response_t response = {0};

    if (socketq_receive_nonblock(&responseq, (uint8_t *)&response, 0)) {
        if (response.error) {
            if (response.error_callback != NULL) {
                response.error_callback(pmodel, response.arg);
            }
        } else {
            response.callback(pmodel, response.data, response.arg);
            if (!response.transfer_data) {
                free(response.data);
            }
        }
        return 1;
    } else {
        return 0;
    }
}


int disk_op_is_firmware_present(void) {
    return storage_is_file(APP_UPDATE);
}


static void *disk_interaction_task(void *args) {
    unsigned int mount_attempts = 0;

    storage_create_dir(DEFAULT_PROGRAMS_PATH);
    storage_create_dir(DEFAULT_PARAMS_PATH);

    for (;;) {
        disk_op_message_t msg;
        if (socketq_receive_nonblock(&requestq, (uint8_t *)&msg, 500)) {
            disk_op_response_t response = {
                .callback       = msg.callback,
                .error_callback = msg.error_callback,
                .data           = NULL,
                .arg            = msg.arg,
                .transfer_data  = 0,
            };

            switch (msg.code) {
                case DISK_OP_MESSAGE_CODE_IMPORT_CURRENT_MACHINE: {
                    if (CONFIG_DATA_VERSION != storage_read_archive_data_version(DRIVE_MOUNT_PATH, msg.data)) {
                        response.error = 1;
                    } else {
                        response.error = storage_load_current_machine_config(DRIVE_MOUNT_PATH, msg.data);
                    }
                    free(msg.data);
                    socketq_send(&responseq, (uint8_t *)&response);
                    break;
                }

                case DISK_OP_MESSAGE_CODE_EXPORT_CURRENT_MACHINE:
                    response.error = storage_save_current_machine_config(DRIVE_MOUNT_PATH, msg.data);
                    free(msg.data);

                    // Refresh saved machines on the drive
                    pthread_mutex_lock(&sem);
                    drive_machines_num = storage_list_saved_machines(DRIVE_MOUNT_PATH, &drive_machines);
                    pthread_mutex_unlock(&sem);

                    socketq_send(&responseq, (uint8_t *)&response);
                    break;

                case DISK_OP_MESSAGE_CODE_READ_FILE: {
                    response.data          = storage_read_file(msg.data);
                    response.transfer_data = 1;
                    free(msg.data);
                    socketq_send(&responseq, (uint8_t *)&response);
                    break;
                }

                case DISK_OP_MESSAGE_CODE_SAVE_PROGRAM_INDEX: {
                    disk_op_name_list_t *list = msg.data;
                    response.error = storage_update_program_index(DEFAULT_PROGRAMS_PATH, list->names, list->num);
                    free(list);
                    socketq_send(&responseq, (uint8_t *)&response);
                    break;
                }

                case DISK_OP_MESSAGE_CODE_REMOVE_PROGRAM:
                    storage_remove_program(DEFAULT_PROGRAMS_PATH, msg.data);
                    free(msg.data);
                    socketq_send(&responseq, (uint8_t *)&response);
                    break;

                case DISK_OP_MESSAGE_CODE_SAVE_PASSWORD:
                    response.error = storage_write_file(DEFAULT_PATH_FILE_PASSWORD, msg.data, strlen(msg.data));
                    free(msg.data);
                    socketq_send(&responseq, (uint8_t *)&response);
                    break;

                case DISK_OP_MESSAGE_CODE_SAVE_PROGRAM:
                    response.error = storage_update_program(DEFAULT_PROGRAMS_PATH, msg.data);
                    free(msg.data);
                    socketq_send(&responseq, (uint8_t *)&response);
                    break;

                case DISK_OP_MESSAGE_CODE_SAVE_PARMAC:
                    response.error = storage_save_parmac(DEFAULT_PATH_FILE_PARMAC, msg.data);
                    free(msg.data);
                    socketq_send(&responseq, (uint8_t *)&response);
                    break;

                case DISK_OP_MESSAGE_CODE_LOAD_PARMAC:
                    response.data = malloc(sizeof(parmac_t));
                    if (response.data == NULL) {
                        response.error = 1;
                    } else {
                        response.error = storage_load_parmac(DEFAULT_PATH_FILE_PARMAC, response.data);
                    }
                    socketq_send(&responseq, (uint8_t *)&response);
                    break;

                case DISK_OP_MESSAGE_CODE_LOAD_PROGRAMS:
                    response.data = malloc(sizeof(storage_program_list_t));
                    if (response.data == NULL) {
                        response.error = 1;
                    } else {
                        response.error = storage_load_saved_programs(DEFAULT_PROGRAMS_PATH, response.data);
                    }
                    socketq_send(&responseq, (uint8_t *)&response);
                    break;

                case DISK_OP_MESSAGE_CODE_SAVE_WIFI_CONFIG:
                    response.error = wifi_save_config();
                    socketq_send(&responseq, (uint8_t *)&response);
                    break;

                case DISK_OP_MESSAGE_CODE_FIRMWARE_UPDATE:
                    response.error = storage_update_temporary_firmware(APP_UPDATE, TEMPORARY_APP);
                    socketq_send(&responseq, (uint8_t *)&response);
                    break;
            }
        }

        if (storage_get_file_size(LOGFILE) > MAX_LOGFILE_SIZE) {
            storage_clear_file(LOGFILE);
        }

        int drive_already_mounted = disk_op_is_drive_mounted();
        int drive_plugged         = storage_is_drive_plugged();

        if (drive_already_mounted && !drive_plugged) {
            pthread_mutex_lock(&sem);
            drive_mounted = 0;
            pthread_mutex_unlock(&sem);
            storage_unmount_drive();
            log_info("Chiavetta rimossa");
            mount_attempts = 0;
        } else if (!drive_already_mounted && drive_plugged) {
            if (mount_attempts < MOUNT_ATTEMPTS) {
                mount_attempts++;
                log_info("Rilevata una chiavetta");
                if (storage_mount_drive() == 0) {
                    pthread_mutex_lock(&sem);
                    drive_mounted      = 1;
                    drive_machines_num = storage_list_saved_machines(DRIVE_MOUNT_PATH, &drive_machines);
                    pthread_mutex_unlock(&sem);
                    // model->system.f_update_ready  = is_firmware_present();
                    log_info("Chiavetta montata con successo");
                    mount_attempts = 0;
                } else {
                    pthread_mutex_lock(&sem);
                    drive_mounted = 0;
                    pthread_mutex_unlock(&sem);
                    log_warn("Non sono riuscito a montare la chiavetta!");
                }
            }
        }
    }

    pthread_exit(NULL);
    return NULL;
}


static void simple_request(int code, disk_op_callback_t cb, disk_op_error_callback_t errcb, void *arg) {
    disk_op_message_t msg = {
        .code           = code,
        .data           = NULL,
        .callback       = cb,
        .error_callback = errcb,
        .arg            = arg,
    };
    socketq_send(&requestq, (uint8_t *)&msg);
}
