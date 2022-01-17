#ifndef STORAGE_H_INCLUDED
#define STORAGE_H_INCLUDED


#include "model/model.h"


typedef struct {
    size_t          num_programs;
    dryer_program_t programs[MAX_PROGRAMS];
} storage_program_list_t;


int    storage_save_parmac(char *path, parmac_t *parmac);
int    storage_load_parmac(char *path, parmac_t *parmac);
int    storage_load_saved_programs(const char *path, storage_program_list_t *pmodel);
int    storage_update_program_index(const char *path, name_t *names, size_t len);
int    storage_update_program(const char *path, dryer_program_t *p);
void   storage_remove_program(char *path, char *name);
char  *storage_read_file(char *name);
size_t storage_get_file_size(const char *path);
void   storage_clear_file(const char *path);
char   storage_write_file(char *path, char *content, size_t len);



#endif