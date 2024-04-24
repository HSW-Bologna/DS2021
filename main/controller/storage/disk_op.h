#ifndef DISK_OP_H_INCLUDED
#define DISK_OP_H_INCLUDED


#include "model/model.h"


typedef void (*disk_op_callback_t)(model_t *, void *, void *);
typedef void (*disk_op_error_callback_t)(model_t *, void *);


typedef struct {
    disk_op_callback_t       callback;
    disk_op_error_callback_t error_callback;
    int                      error;
    void                    *data;
    void                    *arg;
    int                      transfer_data;
} disk_op_response_t;


void   disk_op_init(void);
void   disk_op_load_parmac(disk_op_callback_t cb, disk_op_error_callback_t errcb, void *arg);
void   disk_op_load_programs(disk_op_callback_t cb, disk_op_error_callback_t errcb, void *arg);
void   disk_op_save_parmac(parmac_t *parmac, disk_op_callback_t cb, disk_op_error_callback_t errcb, void *arg);
void   disk_op_save_program_index(model_t *pmodel, disk_op_callback_t cb, disk_op_error_callback_t errcb, void *arg);
void   disk_op_save_program(dryer_program_t *p, disk_op_callback_t cb, disk_op_error_callback_t errcb, void *arg);
int    disk_op_manage_response(model_t *pmodel);
void   disk_op_remove_program(char *filename, disk_op_callback_t cb, disk_op_error_callback_t errcb, void *arg);
void   disk_op_save_wifi_config(disk_op_callback_t cb, disk_op_error_callback_t errcb, void *arg);
void   disk_op_read_file(char *name, disk_op_callback_t cb, disk_op_error_callback_t errcb, void *arg);
void   disk_op_save_password(char *password, disk_op_callback_t cb, disk_op_error_callback_t errcb, void *arg);
int    disk_op_is_drive_mounted(void);
size_t disk_op_drive_machines(name_t **machines);
void   disk_op_export_current_machine(char *name, disk_op_callback_t cb, disk_op_error_callback_t errcb, void *arg);
void   disk_op_import_current_machine(char *name, disk_op_callback_t cb, disk_op_error_callback_t errcb, void *arg);
int    disk_op_is_firmware_present(void);
void   disk_op_firmware_update(disk_op_callback_t cb, disk_op_error_callback_t errcb, void *arg);


#endif
