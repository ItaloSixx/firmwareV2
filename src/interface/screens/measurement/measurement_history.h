#ifndef MEASUREMENT_HISTORY_H
#define MEASUREMENT_HISTORY_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*measurement_history_close_cb_t)(void);

lv_obj_t *measurement_history_create(lv_obj_t *parent, measurement_history_close_cb_t close_cb);

#ifdef __cplusplus
}
#endif

#endif // MEASUREMENT_HISTORY_H
