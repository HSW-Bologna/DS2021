#ifndef VIEW_TYPES_H_INCLUDED
#define VIEW_TYPES_H_INCLUDED

#include "lvgl.h"


#define VIEW_NULL_MESSAGE ((view_message_t){.cmsg = {0}, .vmsg = {0}})


typedef struct {
    int id;
    int number;
} view_object_data_t;



typedef enum {
    PTYPE_DROPDOWN,
    PTYPE_SWITCH,
    PTYPE_NUMBER,
    PTYPE_TIME,
} parameter_display_type_t;


typedef struct {
    const char             **desc;
    const char            ***values;
    const char              *fmt;
    parameter_display_type_t t;
} pudata_t;


typedef enum {
    VIEW_EVENT_CODE_LVGL,
    VIEW_EVENT_CODE_TIMER,
    VIEW_EVENT_CODE_ALARM,
    VIEW_EVENT_CODE_STATE_CHANGED,
    VIEW_EVENT_CODE_TEST_INPUT_VALUES,
    VIEW_EVENT_CODE_OPEN,
} view_event_code_t;


typedef struct {
    view_event_code_t code;

    union {
        int timer_code;
        struct {
            lv_event_code_t    event;
            view_object_data_t data;
            int                value;
            const char        *string_value;
        };

        struct {
            uint8_t digital_inputs;
        };
    };
} view_event_t;



typedef enum {
    VIEW_CONTROLLER_MESSAGE_CODE_NOTHING = 0,
    VIEW_CONTROLLER_MESSAGE_TEST_RELE,
    VIEW_CONTROLLER_MESSAGE_CODE_START_MACHINE,
    VIEW_CONTROLLER_MESSAGE_CODE_STOP_MACHINE,
    VIEW_CONTROLLER_MESSAGE_CODE_RESTART_COMMUNICATION,
} view_controller_message_code_t;


typedef struct {
    view_controller_message_code_t code;

    union {
        struct {
            size_t rele;
            int    value;
        };
    };
} view_controller_message_t;


typedef enum {
    VIEW_PAGE_MESSAGE_CODE_NOTHING = 0,
    VIEW_PAGE_MESSAGE_CODE_CHANGE_PAGE,
    VIEW_PAGE_MESSAGE_CODE_CHANGE_PAGE_EXTRA,
    VIEW_PAGE_MESSAGE_CODE_REBASE,
    VIEW_PAGE_MESSAGE_CODE_BACK,
} view_page_message_code_t;


typedef struct {
    view_page_message_code_t code;

    union {
        struct {
            void *page;
            void *extra;
        };
    };
} view_page_message_t;


typedef struct {
    view_controller_message_t cmsg;
    view_page_message_t       vmsg;
} view_message_t;

#endif