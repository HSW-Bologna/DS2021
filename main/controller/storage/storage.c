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

#define DIR_CHECK(x)                                                                                                   \
    {                                                                                                                  \
        int res = x;                                                                                                   \
        if (res < 0)                                                                                                   \
            log_error("Errore nel maneggiare una cartella: %s", strerror(errno));                                      \
    }

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
#define BASENAME(x) (strrchr(x, '/') + 1)


static int   is_dir(const char *path);
static int   dir_exists(char *name);
static int   is_drive(const char *path);
static char *nth_strrchr(const char *str, char c, int n);
static int   count_occurrences(const char *str, char c);
static void  add_entry_from_data(struct archive *a, struct archive_entry *entry, uint8_t *data, size_t len, char *name);
static void  add_entry_from_path(struct archive *a, struct archive_entry *entry, char *path, char *name);
static int   copy_archive(struct archive *ar, struct archive *aw);
static int   copy_file(const char *to, const char *from);


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
    if (fp == NULL) {
        log_error("Unable to open %s: %s", filename, strerror(errno));
    }
    if (fwrite(buffer, 1, size, fp) == 0) {
        res = 1;
        log_error("Non sono riuscito a scrivere il file %s : %s", filename, strerror(errno));
    }
    log_info("Salvato programma %s con %i step", p->nomi[0], p->num_steps);

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

        if (storage_is_file(file_path)) {
            log_info("Trovato lavaggio %s", file_path);
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
                log_info("Prog %zu with %i steps", count, list->programs[count].num_steps);

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


int storage_list_saved_machines(char *location, name_t **names) {
    int            count = 0, num = 0;
    struct dirent *dir;
    char           path[300];

    DIR *d = opendir(location);

    if (d != NULL) {
        while ((dir = readdir(d)) != NULL) {
            snprintf(path, sizeof(path), "%s/%s", location, dir->d_name);
            if (storage_is_file(path)) {
                num++;
            }
        }

        *names = realloc(*names, sizeof(name_t) * num);
        memset(*names, 0, sizeof(name_t) * num);
        rewinddir(d);

        while ((dir = readdir(d)) != NULL && count < num) {
            snprintf(path, sizeof(path), "%s/%s", location, dir->d_name);

            if (!storage_is_file(path)) {
                continue;
            }

            char *ext = nth_strrchr(dir->d_name, '.', count_occurrences(ARCHIVE_EXTENSION, '.'));
            if (strcmp(ext, ARCHIVE_EXTENSION)) {
                continue;
            }

            int len = strlen(dir->d_name) - strlen(ARCHIVE_EXTENSION);
            len     = len > MAX_NAME_SIZE ? MAX_NAME_SIZE : len;
            memcpy((*names)[count], dir->d_name, len);
            count++;
        }
        closedir(d);
    }

    return count;
}


int storage_save_current_machine_config(const char *destination, const char *name) {
    struct archive       *a;
    struct archive_entry *entry;
    char                  path[300], filename[256], buffer[17];
    char                 *names[MAX_PROGRAMS];

    int num = storage_list_saved_programs(DEFAULT_PROGRAMS_PATH, names);

    remount_rw();

    snprintf(path, sizeof(path), "%s/%s%s", destination, name, ARCHIVE_EXTENSION);
    a = archive_write_new();
    archive_write_add_filter_gzip(a);
    archive_write_set_format_pax_restricted(a);     // Note 1
    if (archive_write_open_filename(a, path) != ARCHIVE_OK) {
        log_error("Errore nell'aprire l'archivio: %s", archive_error_string(a));
        remount_ro();
        return 1;
    }
    entry = archive_entry_new();     // Note 2

    sprintf(filename, "%s", BASENAME(DEFAULT_PATH_FILE_DATA_VERSION));
    snprintf(buffer, 16, "%i", CONFIG_DATA_VERSION);
    add_entry_from_data(a, entry, (uint8_t *)buffer, strlen(buffer), filename);

    sprintf(filename, "%s/%s", BASENAME(DEFAULT_PROGRAMS_PATH), INDEX_FILE_NAME);
    add_entry_from_path(a, entry, DEFAULT_PATH_FILE_INDEX, filename);

    sprintf(filename, "%s/%s", BASENAME(DEFAULT_PARAMS_PATH), BASENAME(DEFAULT_PATH_FILE_PARMAC));
    add_entry_from_path(a, entry, DEFAULT_PATH_FILE_PARMAC, filename);

    for (int i = 0; i < num; i++) {
        sprintf(path, "%s/%s", DEFAULT_PROGRAMS_PATH, names[i]);
        sprintf(filename, "%s/%s", BASENAME(DEFAULT_PROGRAMS_PATH), names[i]);
        add_entry_from_path(a, entry, path, filename);
        free(names[i]);
    }

    archive_entry_free(entry);
    archive_write_close(a);
    archive_write_free(a);
    sync();

    remount_ro();

    return 0;
}


int storage_read_archive_data_version(const char *location, const char *name) {
    struct archive       *a;
    struct archive_entry *entry;
    int                   r, res = 0;
    char                  path[256];

    remount_rw();

    a = archive_read_new();
    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);

    sprintf(path, "%s/%s%s", location, name, ARCHIVE_EXTENSION);
    if ((r = archive_read_open_filename(a, path, 10240))) {
        log_error("Non sono riuscito ad aprire l'archivio: %s\n", archive_error_string(a));
        remount_ro();
        return -1;
    }

    for (;;) {
        r                = archive_read_next_header(a, &entry);
        const char *path = archive_entry_pathname(entry);
        if (r == ARCHIVE_EOF)
            break;
        if (r < ARCHIVE_OK)
            log_warn("%s\n", archive_error_string(a));
        if (r < ARCHIVE_WARN)
            break;

        if (strcmp(path, BASENAME(DEFAULT_PATH_FILE_DATA_VERSION)))
            continue;

        char buffer[17] = {0};
        int  len        = archive_read_data(a, buffer, 16);
        if (len > 0) {
            res = atoi(buffer);
            break;
        } else {
            log_warn("Non sono riuscito a leggere la versione dei dati: %s\n", archive_error_string(a));
            res = -1;
            break;
        }
    }
    archive_read_close(a);
    archive_read_free(a);

    remount_ro();

    return res;
}


int storage_load_current_machine_config(const char *location, const char *name) {
    struct archive       *a;
    struct archive       *ext;
    struct archive_entry *entry;

    int  r;
    char newname[256];
    char path[256];

    remount_rw();

    a = archive_read_new();
    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);
    ext = archive_write_disk_new();
    archive_write_disk_set_options(ext, 0);
    // archive_write_disk_set_standard_lookup(ext);

    sprintf(path, "%s/%s%s", location, name, ARCHIVE_EXTENSION);
    if ((r = archive_read_open_filename(a, path, 10240))) {
        log_error("Non sono riuscito ad aprire l'archivio: %s\n", archive_error_string(a));
        remount_ro();
        return -1;
    }

    for (;;) {
        r = archive_read_next_header(a, &entry);

        if (r == ARCHIVE_EOF)
            break;
        if (r < ARCHIVE_OK)
            log_warn("%s\n", archive_error_string(a));
        if (r < ARCHIVE_WARN)
            break;

        const char *path = archive_entry_pathname(entry);

        if (strcmp(path, BASENAME(DEFAULT_PATH_FILE_DATA_VERSION)) == 0) {
            continue;
        }

        sprintf(newname, "%s/%s", DEFAULT_BASE_PATH, path);
        archive_entry_set_pathname(entry, newname);

        r = archive_write_header(ext, entry);
        if (r < ARCHIVE_OK)
            log_warn("%s\n", archive_error_string(ext));

        else if (archive_entry_size(entry) > 0) {
            r = copy_archive(a, ext);
            if (r < ARCHIVE_OK) {
                log_warn("%s\n", archive_error_string(ext));
            }
            if (r < ARCHIVE_WARN) {
                break;
            }
        }
        r = archive_write_finish_entry(ext);
        if (r < ARCHIVE_OK) {
            log_warn("%s\n", archive_error_string(ext));
        }
        if (r < ARCHIVE_WARN) {
            break;
        }
    }
    archive_read_close(a);
    archive_read_free(a);
    archive_write_close(ext);
    archive_write_free(ext);

    remount_ro();

    return 0;
}


/*
 *  Chiavetta USB
 */

char storage_is_drive_plugged(void) {
#ifdef TARGET_DEBUG
    return is_dir(DRIVE_MOUNT_PATH);
#else
    char drive[32];
    for (char c = 'a'; c < 'h'; c++) {
        snprintf(drive, sizeof(drive), "/dev/sd%c", c);
        if (is_drive(drive))
            return c;
    }
    return 0;
#endif
}


int storage_mount_drive(void) {
#ifdef TARGET_DEBUG
    return 0;
#else
    char path[80];
    char drive[64];

    if (!dir_exists(DRIVE_MOUNT_PATH)) {
        storage_create_dir(DRIVE_MOUNT_PATH);
    }

    char c = storage_is_drive_plugged();
    if (!c) {
        return -1;
    }

    snprintf(drive, sizeof(drive), "/dev/sd%c", c);
    snprintf(path, sizeof(path), "%s1", drive);
    if (!is_drive(path)) {     // Se presente monta la prima partizione
        strcpy(path, drive);
        log_info("Non trovo una partizione; monto %s", path);
    } else {
        log_info("Trovata partizione %s", path);
    }

    if (mount(path, DRIVE_MOUNT_PATH, "vfat", 0, NULL) < 0) {
        if (errno == EBUSY)     // Il file system era gia' montato
            return 0;

        log_error("Errore nel montare il disco %s: %s (%i)", path, strerror(errno), errno);
        return -1;
    }
    return 0;
#endif
}


void storage_unmount_drive(void) {
#ifdef TARGET_DEBUG
    return;
#endif
    if (dir_exists(DRIVE_MOUNT_PATH)) {
        if (umount(DRIVE_MOUNT_PATH) < 0) {
            log_warn("Umount error: %s (%i)", strerror(errno), errno);
        }
        rmdir(DRIVE_MOUNT_PATH);
    }
}


int storage_is_file(const char *path) {
    struct stat path_stat;
    if (stat(path, &path_stat) < 0)
        return 0;
    return S_ISREG(path_stat.st_mode);
}


int storage_update_temporary_firmware(char *app_path, char *temporary_path) {
    int res = 0;

#ifdef TARGET_DEBUG
    return 0;
#endif

    if (storage_is_file(app_path)) {
        if (mount("/dev/root", "/", "ext2", MS_REMOUNT, NULL) < 0) {
            log_error("Errore nel montare la root: %s (%i)", strerror(errno), errno);
            return -1;
        }

        if ((res = copy_file(temporary_path, app_path)) < 0)
            log_error("Non sono riuscito ad aggiornare il firmware");

        if (mount("/dev/root", "/", "ext2", MS_REMOUNT | MS_RDONLY, NULL) < 0)
            log_warn("Errore nel rimontare la root: %s (%i)", strerror(errno), errno);

        sync();
    } else {
        return -1;
    }

    return res;
}


/*
 *  Static functions
 */


static int is_dir(const char *path) {
    struct stat path_stat;
    if (stat(path, &path_stat) < 0)
        return 0;
    return S_ISDIR(path_stat.st_mode);
}


static int is_drive(const char *path) {
    struct stat path_stat;
    if (stat(path, &path_stat) < 0)
        return 0;
    return S_ISBLK(path_stat.st_mode);
}


static int dir_exists(char *name) {
    DIR *dir = opendir(name);
    if (dir) {
        closedir(dir);
        return 1;
    }
    return 0;
}


static char *nth_strrchr(const char *str, char c, int n) {
    const char *s = &str[strlen(str) - 1];

    while ((n -= (*s == c)) && (s != str)) {
        s--;
    }

    return (char *)s;
}


static int count_occurrences(const char *str, char c) {
    int i = 0;
    for (i = 0; str[i]; str[i] == c ? i++ : *str++)
        ;
    return i;
}


/*
 *  Archivio di configurazioni macchina
 */

static void add_entry_from_data(struct archive *a, struct archive_entry *entry, uint8_t *data, size_t len, char *name) {
    archive_entry_set_pathname(entry, name);
    archive_entry_set_size(entry, len);
    archive_entry_set_filetype(entry, AE_IFREG);
    archive_entry_set_perm(entry, 0644);
    if (archive_write_header(a, entry) != ARCHIVE_OK) {
        log_warn("Errore nella creazione dell'archivio: %s", archive_error_string(a));
        return;
    }

    if (archive_write_data(a, data, len) < 0)
        log_warn("Errore nella scrittura dell'archivio: %s", archive_error_string(a));

    archive_entry_clear(entry);
}



static void add_entry_from_path(struct archive *a, struct archive_entry *entry, char *path, char *name) {
    char        buff[2048];
    int         len, fd;
    struct stat st;

    stat(path, &st);
    archive_entry_set_pathname(entry, name);
    archive_entry_set_size(entry, st.st_size);
    archive_entry_set_filetype(entry, AE_IFREG);
    archive_entry_set_perm(entry, 0644);
    if (archive_write_header(a, entry) != ARCHIVE_OK) {
        log_warn("Errore nella creazione dell'archivio: %s", archive_error_string(a));
        return;
    }

    if ((fd = open(path, O_RDONLY)) < 0) {
        log_warn("Non riesco ad aprire il file %s:%s", path, strerror(errno));
        return;
    }

    len = read(fd, buff, sizeof(buff));
    while (len > 0) {
        if (archive_write_data(a, buff, len) < 0) {
            log_warn("Errore nella scrittura dell'archivio: %s", archive_error_string(a));
            break;
        }
        len = read(fd, buff, sizeof(buff));
    }
    close(fd);
    archive_entry_clear(entry);
}


static int copy_archive(struct archive *ar, struct archive *aw) {
    int         r;
    const void *buff;
    size_t      size;
#if ARCHIVE_VERSION_NUMBER >= 3000000
    int64_t offset;
#else
    off_t offset;
#endif

    for (;;) {
        r = archive_read_data_block(ar, &buff, &size, &offset);
        if (r == ARCHIVE_EOF)
            return (ARCHIVE_OK);
        if (r != ARCHIVE_OK)
            return (r);
        r = archive_write_data_block(aw, buff, size, offset);
        if (r != ARCHIVE_OK) {
            printf("archive_write_data_block(): %s", archive_error_string(aw));
            return (r);
        }
    }
}


void storage_create_dir(char *name) {
    remount_rw();
    DIR_CHECK(mkdir(name, 0766));
    remount_ro();
}


static int copy_file(const char *to, const char *from) {
    int     fd_to, fd_from;
    char    buf[4096];
    ssize_t nread;

    fd_from = open(from, O_RDONLY);
    if (fd_from < 0) {
        log_warn("Non sono riuscito ad aprire %s: %s", from, strerror(errno));
        return -1;
    }

    fd_to = open(to, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd_to < 0) {
        log_warn("Non sono riuscito ad aprire %s: %s", to, strerror(errno));
        close(fd_from);
        return -1;
    }

    while ((nread = read(fd_from, buf, sizeof buf)) > 0) {
        char   *out_ptr = buf;
        ssize_t nwritten;

        do {
            nwritten = write(fd_to, out_ptr, nread);

            if (nwritten >= 0) {
                nread -= nwritten;
                out_ptr += nwritten;
            } else {
                close(fd_from);
                close(fd_to);
                return -1;
            }
        } while (nread > 0);
    }

    close(fd_to);
    close(fd_from);

    return 0;
}
