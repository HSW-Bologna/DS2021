#include <stdlib.h>
#include <getopt.h>
#include "args.h"


static args_flags_t parse_args(int argc, char *argv[]) {
    args_flags_t flags        = 0;
    int          flag_version = 0;

    struct option long_options[] = {
        /* These options set a flag. */
        {"version", no_argument, &flag_version, 1},
        {0, 0, 0, 0},
    };
    int opt;

    while ((opt = getopt_long(argc, argv, "cmu:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'm':
                flags |= 1 << ARGS_MUTED_BIT;
                break;

            case 'c':
                flags |= 1 << ARGS_CRASHED_BIT;
                break;

            case 'u':
                // model->args.fw_new_location = malloc(strlen(optarg) + 1);
                // strcpy(model->args.fw_new_location, optarg);
                flags |= 1 << ARGS_UPDATE_BIT;
                break;

            default:
                break;
        }
    }

    if (flag_version) {
        flags |= 1 << ARGS_VERSION_BIT;
    }

    return flags;
}
