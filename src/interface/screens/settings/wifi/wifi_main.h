#ifndef WIFI_MAIN_H
#define WIFI_MAIN_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration para callback de voltar
typedef void (*wifi_back_cb_t)(void);

lv_obj_t *wifi_main_create(lv_obj_t *parent);

void wifi_main_set_back_callback(wifi_back_cb_t callback);
void wifi_main_service_init(void);

// Funções para preservar Wi-Fi
void wifi_main_preserve_connection(void);
void wifi_main_restore_connection(void);
bool wifi_main_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif // WIFI_MAIN_H
