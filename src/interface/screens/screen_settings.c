/**
 * @file screen_settings.c
 * @brief Wrapper para compatibilidade com a tela de configurações
 * @author ItaloSixx
 * @date 2025
 */

#include "screen_settings.h"
#include "settings/settings_main.h"

lv_obj_t *screen_settings_create(lv_obj_t *parent)
{
    return settings_main_create(parent);
}

void screen_settings_update(lv_obj_t *screen)
{
    settings_main_update(screen);
}

void screen_settings_destroy(lv_obj_t *screen)
{
    settings_main_destroy(screen);
}

// Funções para compatibilidade - redirecionam para as novas implementações
uint8_t settings_get_brightness(void) { 
    return settings_main_get_brightness(); 
}

bool settings_get_night_mode(void) { 
    return settings_main_get_night_mode(); 
}

bool settings_get_wifi_enabled(void) { 
    return settings_main_get_wifi_enabled(); 
}

bool settings_get_bluetooth_enabled(void) { 
    return settings_main_get_bluetooth_enabled(); 
}

void settings_set_brightness(uint8_t value) { 
    settings_main_set_brightness(value); 
}

void settings_set_night_mode(bool enabled) { 
    settings_main_set_night_mode(enabled); 
}

void settings_set_wifi_enabled(bool enabled) { 
    settings_main_set_wifi_enabled(enabled); 
}

void settings_set_bluetooth_enabled(bool enabled) { 
    settings_main_set_bluetooth_enabled(enabled); 
}