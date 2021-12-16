#ifndef GEL_PMAN_CONF_H_INCLUDED
#define GEL_PMAN_CONF_H_INCLUDED

#include "model/model.h"
#include "view/view_types.h"

/*
 * Page manager
 */
#define PMAN_NAVIGATION_DEPTH 8
#define PMAN_VIEW_NULL
#define PMAN_DATA_NULL NULL

typedef view_message_t pman_message_t;

typedef view_event_t pman_event_t;

typedef void *pman_page_data_t;

typedef model_t *pman_model_t;

typedef void pman_view_t;


#endif
