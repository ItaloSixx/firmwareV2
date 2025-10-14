#ifndef UI_OS_H
#define UI_OS_H

#include "ui_types.h"
#include "../sensors/sensors.h"

void ui_os_init(void);
void ui_set_screen(ui_screen_t screen);
ui_screen_t ui_get_current_screen(void);
void ui_os_update(void);
void ui_update_sensor_data(const sensor_data_t *data);
void ui_update_system_state(const ui_system_state_t *state);
void ui_update_system_stats(const ui_system_stats_t *stats);
void ui_show_notification(const char *message, const char *type);

#endif