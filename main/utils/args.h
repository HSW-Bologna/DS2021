#ifndef ARGS_H_INCLUDED
#define ARGS_H_INCLUDED


#include <stdint.h>


#define ARGS_VERSION_BIT 0
#define ARGS_MUTED_BIT   1
#define ARGS_CRASHED_BIT 2
#define ARGS_UPDATE_BIT  3

#define ARGS_VERSION(flags) ((flags & (1 << ARGS_VERSION_BIT)) > 0)
#define ARGS_MUTED(flags)   ((flags & (1 << ARGS_MUTED_BIT)) > 0)
#define ARGS_CRASHED(flags) ((flags & (1 << ARGS_CRASHED_BIT)) > 0)
#define ARGS_UPDATE(flags)  ((flags & (1 << ARGS_UPDATE_BIT)) > 0)


typedef uint32_t args_flags_t;


#endif