#include <unistd.h>
#include <archive.h>
#include <archive_entry.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mount.h>
#include "errno.h"
#include "model/model.h"
#include "storage.h"
#include "log.h"
#include "config/app_conf.h"


#ifdef TARGET_DEBUG
#define remount_ro()
#define remount_rw()
#else
static inline void remount_ro() {
    if (mount("/dev/mmcblk0p3", DEFAULT_BASE_PATH, "ext2", MS_REMOUNT | MS_RDONLY, NULL) < 0)
        log_error("Errore nel montare la partizione %s: %s (%i)", DEFAULT_BASE_PATH, strerror(errno), errno);
}

static inline void remount_rw() {
    if (mount("/dev/mmcblk0p3", DEFAULT_BASE_PATH, "ext2", MS_REMOUNT, NULL) < 0)
        log_error("Errore nel montare la partizione %s: %s (%i)", DEFAULT_BASE_PATH, strerror(errno), errno);
}
#endif


static int is_file(const char *path);


int storage_load_parmac(char *path, parmac_t *parmac) {
    uint8_t buffer[PARMAC_SIZE] = {0};
    int     res                 = 0;
    FILE   *f                   = fopen(path, "r");

    if (!f) {
        log_info("Parametri macchina non trovati");
        return -1;
    } else {
        if ((res = fread(buffer, 1, PARMAC_SIZE, f)) == 0) {
            log_warn("Errore nel caricamento dei parametri macchina: %s", strerror(errno));
        }
        fclose(f);

        if (res == 0) {
            return -1;
        } else {
            model_deserialize_parmac(parmac, buffer);
            return 0;
        }
    }
}


int storage_save_parmac(char *path, parmac_t *parmac) {
    uint8_t buffer[PARMAC_SIZE] = {0};
    int     res                 = 0;

    size_t size = model_serialize_parmac(buffer, parmac);

    remount_rw();

    FILE *f = fopen(path, "w");
    if (!f) {
        log_warn("Non riesco ad aprire %s in scrittura: %s", path, strerror(errno));
        res = 1;
    } else {
        if (fwrite(buffer, 1, size, f) == 0) {
            log_warn("Non riesco a scrivere i parametri macchina: %s", strerror(errno));
            res = 1;
        }
        fclose(f);
    }

    remount_ro();

    return res;
}


void storage_remove_program(char *path, char *name) {
    char filename[128];
    snprintf(filename, sizeof(filename), "%s/%s", path, name);

    remount_rw();

    if (remove(filename)) {
        log_warn("Non sono riuscito a cancellare il file %s: %s", filename, strerror(errno));
    }

    remount_ro();
}


int storage_update_program(const char *path, dryer_program_t *p) {
    char    filename[128];
    uint8_t buffer[MAX_PROGRAM_SIZE];
    int     res = 0;

    size_t size = program_serialize(buffer, p);

    remount_rw();

    snprintf(filename, sizeof(filename), "%s/%s", path, p->filename);
    FILE *fp = fopen(filename, "w");
    if (fwrite(buffer, 1, size, fp) == 0) {
        res = 1;
        log_error("Non sono riuscito a scrivere il file %s : %s", filename, strerror(errno));
    }

    fclose(fp);
    remount_ro();
    return res;
}


int storage_update_program_index(const char *path, name_t *names, size_t len) {
    int  res = 0;
    char filename[128];
    snprintf(filename, sizeof(filename), "%s/%s", path, INDEX_FILE_NAME);
    remount_rw();

    FILE *findex = fopen(filename, "w");

    if (!findex) {
        log_warn("Operazione di scrittura dell'indice fallita: %s", strerror(errno));
        res = 1;
    } else {
        char buffer[64];

        for (size_t i = 0; i < len; i++) {
            if (strlen(names[i]) == 0) {
                continue;
            }

            snprintf(buffer, STRING_NAME_SIZE + 1, "%s\n", names[i]);
            if (fwrite(buffer, 1, strlen(buffer), findex) == 0) {
                log_warn("Errore durante la scrittura dell'indice dei programmi: %s", strerror(errno));
                res = 1;
            }
        }
        fclose(findex);
    }
    remount_ro();

    return res;
}


void storage_clear_orphan_programs(char *path, dryer_program_t *ps, int num) {
    struct dirent *dir;
    char           string[300];

    remount_rw();

    DIR *d = opendir(path);

    if (d) {
        while ((dir = readdir(d)) != NULL) {
            int orphan = 1;

            if (strcmp(dir->d_name, INDEX_FILE_NAME) == 0)
                continue;

            for (int i = 0; i < num; i++) {
                if (strcmp(ps[i].filename, dir->d_name) == 0) {
                    orphan = 0;
                    break;
                }
            }

            if (orphan) {
                snprintf(string, sizeof(string), "%s/%s", path, dir->d_name);
                remove(string);
            }
        }
        closedir(d);
    }

    remount_ro();
}


int storage_list_saved_programs(const char *path, char *names[]) {
    char filename[128] = {0};
    int  count         = 0;

    snprintf(filename, sizeof(filename), "%s/%s", path, INDEX_FILE_NAME);
    FILE *findex = fopen(filename, "r");
    if (!findex) {
        log_info("Indice non trovato: %s", strerror(errno));
        return -1;
    } else {
        char filename[STRING_NAME_SIZE + 1];

        while (fgets(filename, STRING_NAME_SIZE + 1, findex)) {
            int len = strlen(filename);

            if (filename[len - 1] == '\n')     // Rimuovo il newline
                filename[len - 1] = 0;

            names[count] = malloc(strlen(filename) + 1);
            strcpy(names[count++], filename);
        }

        fclose(findex);
    }

    return count;
}


int storage_load_saved_programs(const char *path, storage_program_list_t *list) {
    uint8_t buffer[MAX_PROGRAM_SIZE];
    char   *names[100];
    name_t  filename;
    int     num = storage_list_saved_programs(path, names);
    char    file_path[128];
    int     count = 0;

    if (num < 0) {
        storage_update_program_index(path, NULL, 0);
        num = 0;
    }

    for (int i = 0; i < num; i++) {
        sprintf(file_path, "%s/%s", path, names[i]);
        memset(filename, 0, sizeof(name_t));
        strcpy(filename, names[i]);
        free(names[i]);

        if (is_file(file_path)) {
            log_debug("Trovato lavaggio %s", file_path);
            FILE *fp = fopen(file_path, "r");

            if (!fp) {
                log_error("Non sono riuscito ad aprire il file %s: %s", file_path, strerror(errno));
                continue;
            }

            size_t read = fread(buffer, 1, MAX_PROGRAM_SIZE, fp);

            if (read == 0) {
                log_error("Non sono riuscito a leggere il file %s: %s", file_path, strerror(errno));
            } else {
                program_deserialize(&list->programs[count], buffer);
                memcpy(&list->programs[count].filename, filename, STRING_NAME_SIZE);

                count++;
            }

            fclose(fp);
        }
    }

    list->num_programs = count;
    return 0;
}


char storage_write_file(char *path, char *content, size_t len) {
    int res = 0;

    remount_rw();
    FILE *f = fopen(path, "w");
    if (!f) {
        log_warn("Non riesco ad aprire %s in scrittura: %s", path, strerror(errno));
        res = 1;
    } else {
        if (fwrite(content, 1, len, f) == 0) {
            log_warn("Non riesco a scrivere il file %s: %s", path, strerror(errno));
            res = 1;
        }
        fclose(f);
    }

    remount_ro();

    return res;
}


char *storage_read_file(char *name) {
    unsigned long size = 0;
    char         *r    = NULL;
    FILE         *fp   = fopen(name, "r");

    if (fp != NULL) {
        fseek(fp, 0L, SEEK_END);
        size = ftell(fp);
        rewind(fp);

        r = malloc(size);
        fread(r, sizeof(char), size, fp);
        r[size] = 0;
    } else {
        log_error("Non sono riuscito a leggere il file %s: %s", name, strerror(errno));
    }

    return r;
}


size_t storage_get_file_size(const char *path) {
    struct stat st;
    if (stat(path, &st) < 0) {
        return 0;
    }
    return st.st_size;
}


void storage_clear_file(const char *path) {
    FILE *fp = fopen(path, "w");
    if (fp != NULL) {
        fclose(fp);
    }
}


static int is_file(const char *path) {
    struct stat path_stat;
    if (stat(path, &path_stat) < 0)
        return 0;
    return S_ISREG(path_stat.st_mode);
}
