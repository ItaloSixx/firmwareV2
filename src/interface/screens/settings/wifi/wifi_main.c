#include "wifi_main.h"
#include "lvgl.h"
#include "lv_port.h"
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <esp_log.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <lwip/inet.h>
#include <lwip/ip_addr.h>

static const char *TAG = "WIFI_MAIN";

#define WIFI_NVS_NAMESPACE "wifi_cfg"
#define WIFI_NVS_KEY_SSID   "ssid"
#define WIFI_NVS_KEY_PASS   "pass"
#define WIFI_RECONNECT_DELAY_MS 2000

static lv_obj_t *g_switch = NULL;
static lv_obj_t *g_list = NULL;
static lv_obj_t *g_status_label = NULL;
static lv_obj_t *g_ssid_label = NULL;
static lv_obj_t *g_password_ta = NULL;
static lv_obj_t *g_keyboard = NULL;
static lv_obj_t *g_connect_btn = NULL;
static lv_obj_t *g_screen = NULL;
static bool wifi_enabled = false;
static bool wifi_keep_alive = false;
static bool wifi_connected = false;
static wifi_back_cb_t g_back_callback = NULL;
static bool wifi_service_initialized = false;
static bool wifi_events_registered = false;
static bool wifi_sta_started = false;
static bool g_has_saved_credentials = false;
static char g_current_password[65] = {0};
static esp_netif_t *g_sta_netif = NULL;
static bool wifi_service_started = false;

// Buffer para armazenar redes escaneadas
#define MAX_SCAN_RECORDS 20
static wifi_ap_record_t g_scan_records[MAX_SCAN_RECORDS];
static uint16_t g_scan_count = 0;
static char g_selected_ssid[33] = {0};
static char g_saved_ssid[33] = {0};
static char g_saved_password[65] = {0};
static TickType_t g_last_reconnect_attempt = 0;

static void wifi_switch_event_cb(lv_event_t *e);
static void wifi_connect_event_cb(lv_event_t *e);
static void wifi_list_event_cb(lv_event_t *e);
static void password_event_cb(lv_event_t *e);
static void show_keyboard(lv_obj_t *ta);
static void hide_keyboard(void);
static void keyboard_event_cb(lv_event_t *e);
static void scan_and_list_networks(lv_obj_t *parent);
static void back_button_event_cb(lv_event_t *e);
static esp_err_t wifi_start_stack(void);
static esp_err_t wifi_init(void);
static void wifi_scan_start(void);
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static esp_err_t wifi_connect_internal(const char *ssid, const char *password, bool triggered_by_ui);
static void wifi_save_credentials(const char *ssid, const char *password);
static bool wifi_load_credentials(char *ssid, size_t ssid_len, char *password, size_t pass_len);
static void wifi_clear_credentials(void);
static void ui_set_status(const char *text);
static void ui_set_status_fmt(const char *fmt, ...);
static bool wifi_has_ui(void);
static void wifi_detach_ui(void);
static void wifi_screen_delete_event_cb(lv_event_t *e);

static void wifi_detach_ui(void)
{
    g_screen = NULL;
    g_switch = NULL;
    g_list = NULL;
    g_status_label = NULL;
    g_ssid_label = NULL;
    g_password_ta = NULL;
    g_connect_btn = NULL;
    g_keyboard = NULL;
}

static void wifi_screen_delete_event_cb(lv_event_t *e)
{
    ESP_LOGI(TAG, "Wi-Fi screen being deleted, detach UI references");
    wifi_detach_ui();
}

static bool wifi_has_ui(void)
{
    return g_screen != NULL;
}

static void ui_set_status(const char *text)
{
    if (!wifi_has_ui() || !g_status_label || !text) {
        return;
    }

    if (lvgl_port_lock(50)) {
        if (lv_obj_is_valid(g_status_label)) {
            lv_label_set_text(g_status_label, text);
        }
        lvgl_port_unlock();
    } else {
        ESP_LOGW(TAG, "Não foi possível obter lock LVGL para atualizar status");
    }
}

static void ui_set_status_fmt(const char *fmt, ...)
{
    if (!wifi_has_ui() || !g_status_label || !fmt) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    char buffer[160];
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    if (lvgl_port_lock(50)) {
        if (lv_obj_is_valid(g_status_label)) {
            lv_label_set_text(g_status_label, buffer);
        }
        lvgl_port_unlock();
    } else {
        ESP_LOGW(TAG, "Não foi possível obter lock LVGL para atualizar status formatado");
    }
}

static void wifi_save_credentials(const char *ssid, const char *password)
{
    if (!ssid) {
        return;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(WIFI_NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao abrir NVS para salvar credenciais: %s", esp_err_to_name(err));
        return;
    }

    err = nvs_set_str(handle, WIFI_NVS_KEY_SSID, ssid);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao salvar SSID na NVS: %s", esp_err_to_name(err));
        nvs_close(handle);
        return;
    }

    if (password) {
        err = nvs_set_str(handle, WIFI_NVS_KEY_PASS, password);
    } else {
        err = nvs_set_str(handle, WIFI_NVS_KEY_PASS, "");
    }

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao salvar senha na NVS: %s", esp_err_to_name(err));
        nvs_close(handle);
        return;
    }

    err = nvs_commit(handle);
    nvs_close(handle);

    if (err == ESP_OK) {
        g_has_saved_credentials = true;
        strncpy(g_saved_ssid, ssid, sizeof(g_saved_ssid) - 1);
        g_saved_ssid[sizeof(g_saved_ssid) - 1] = '\0';

        if (password) {
            strncpy(g_saved_password, password, sizeof(g_saved_password) - 1);
            g_saved_password[sizeof(g_saved_password) - 1] = '\0';
        } else {
            g_saved_password[0] = '\0';
        }

        strncpy(g_current_password, g_saved_password, sizeof(g_current_password) - 1);
        g_current_password[sizeof(g_current_password) - 1] = '\0';
        ESP_LOGI(TAG, "Credenciais Wi-Fi salvas com sucesso");
    } else {
        ESP_LOGE(TAG, "Falha ao confirmar credenciais na NVS: %s", esp_err_to_name(err));
    }
}

static bool wifi_load_credentials(char *ssid, size_t ssid_len, char *password, size_t pass_len)
{
    if (!ssid || ssid_len == 0) {
        return false;
    }

    g_has_saved_credentials = false;

    nvs_handle_t handle;
    esp_err_t err = nvs_open(WIFI_NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Nenhuma credencial Wi-Fi salva (erro ao abrir NVS: %s)", esp_err_to_name(err));
        return false;
    }

    size_t required = ssid_len;
    err = nvs_get_str(handle, WIFI_NVS_KEY_SSID, ssid, &required);
    if (err != ESP_OK || required == 0) {
        ESP_LOGW(TAG, "SSID salvo não encontrado na NVS");
        nvs_close(handle);
        return false;
    }

    if (password && pass_len > 0) {
        required = pass_len;
        err = nvs_get_str(handle, WIFI_NVS_KEY_PASS, password, &required);
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            password[0] = '\0';
            err = ESP_OK;
        }
    }

    nvs_close(handle);

    if (err == ESP_OK) {
        g_has_saved_credentials = true;
        strncpy(g_saved_ssid, ssid, sizeof(g_saved_ssid) - 1);
        g_saved_ssid[sizeof(g_saved_ssid) - 1] = '\0';

        if (password) {
            strncpy(g_saved_password, password, sizeof(g_saved_password) - 1);
            g_saved_password[sizeof(g_saved_password) - 1] = '\0';
        } else {
            g_saved_password[0] = '\0';
        }

        strncpy(g_current_password, g_saved_password, sizeof(g_current_password) - 1);
        g_current_password[sizeof(g_current_password) - 1] = '\0';
        strncpy(g_selected_ssid, g_saved_ssid, sizeof(g_selected_ssid) - 1);
        g_selected_ssid[sizeof(g_selected_ssid) - 1] = '\0';
        ESP_LOGI(TAG, "Credenciais Wi-Fi carregadas da NVS");
        return true;
    }

    ESP_LOGW(TAG, "Falha ao carregar credenciais da NVS: %s", esp_err_to_name(err));
    return false;
}

static void wifi_clear_credentials(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(WIFI_NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err == ESP_OK) {
        nvs_erase_key(handle, WIFI_NVS_KEY_SSID);
        nvs_erase_key(handle, WIFI_NVS_KEY_PASS);
        nvs_commit(handle);
        nvs_close(handle);
    }

    g_has_saved_credentials = false;
    memset(g_selected_ssid, 0, sizeof(g_selected_ssid));
    memset(g_current_password, 0, sizeof(g_current_password));
    memset(g_saved_ssid, 0, sizeof(g_saved_ssid));
    memset(g_saved_password, 0, sizeof(g_saved_password));
    ESP_LOGI(TAG, "Credenciais Wi-Fi removidas da NVS");
}

lv_obj_t *wifi_main_create(lv_obj_t *parent)
{
    g_screen = lv_obj_create(parent);
    lv_obj_set_size(g_screen, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(g_screen, lv_color_hex(0xF5F5F5), LV_PART_MAIN);
    lv_obj_set_style_border_width(g_screen, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(g_screen, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(g_screen, wifi_screen_delete_event_cb, LV_EVENT_DELETE, NULL);

    // Botão Voltar
    lv_obj_t *back_btn = lv_btn_create(g_screen);
    lv_obj_set_size(back_btn, 60, 30);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_add_event_cb(back_btn, back_button_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "< Back");
    lv_obj_center(back_label);

    lv_obj_t *title = lv_label_create(g_screen);
    lv_label_set_text(title, "WiFi Settings");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_22, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    g_switch = lv_switch_create(g_screen);
    lv_obj_align(g_switch, LV_ALIGN_TOP_RIGHT, -20, 10);
    lv_obj_add_event_cb(g_switch, wifi_switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_state(g_switch, LV_STATE_CHECKED);

    g_status_label = lv_label_create(g_screen);
    lv_label_set_text(g_status_label, "WiFi desligado");
    lv_obj_align(g_status_label, LV_ALIGN_TOP_LEFT, 20, 50);

    g_list = lv_list_create(g_screen);
    lv_obj_set_size(g_list, 440, 140);
    lv_obj_align(g_list, LV_ALIGN_TOP_MID, 0, 80);
    lv_obj_add_flag(g_list, LV_OBJ_FLAG_HIDDEN);

    g_ssid_label = lv_label_create(g_screen);
    lv_label_set_text(g_ssid_label, "SSID: --");
    lv_obj_align(g_ssid_label, LV_ALIGN_TOP_LEFT, 20, 230);
    lv_obj_add_flag(g_ssid_label, LV_OBJ_FLAG_HIDDEN);

    g_password_ta = lv_textarea_create(g_screen);
    lv_obj_set_size(g_password_ta, 280, 40);
    lv_obj_align(g_password_ta, LV_ALIGN_TOP_LEFT, 20, 250);
    lv_textarea_set_placeholder_text(g_password_ta, "Digite a senha da rede");
    lv_obj_add_event_cb(g_password_ta, password_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(g_password_ta, LV_OBJ_FLAG_HIDDEN);

    g_connect_btn = lv_btn_create(g_screen);
    lv_obj_set_size(g_connect_btn, 100, 40);
    lv_obj_align(g_connect_btn, LV_ALIGN_TOP_LEFT, 310, 250);
    lv_obj_add_event_cb(g_connect_btn, wifi_connect_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(g_connect_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *connect_label = lv_label_create(g_connect_btn);
    lv_label_set_text(connect_label, "Conectar");
    lv_obj_center(connect_label);

    // Restaurar estado da conexão Wi-Fi se estiver preservada
    wifi_main_restore_connection();

    return g_screen;
}

static void wifi_switch_event_cb(lv_event_t *e)
{
    wifi_enabled = lv_obj_has_state(g_switch, LV_STATE_CHECKED);
    if (wifi_enabled) {
        ui_set_status("Iniciando Wi-Fi...");
        if (wifi_init() == ESP_OK) {
            if (g_list) {
                lv_obj_clear_flag(g_list, LV_OBJ_FLAG_HIDDEN);
            }
            /* Pequeno delay para estabilizar antes do scan */
            vTaskDelay(pdMS_TO_TICKS(200));
            wifi_scan_start();
        }
    } else {
        ESP_LOGI(TAG, "Solicitação para desligar Wi-Fi...");
        
        // Se estiver conectado e preservando, não desligar
        if (wifi_connected && wifi_keep_alive) {
            ESP_LOGI(TAG, "Wi-Fi conectado será preservado");
            ui_set_status("Wi-Fi conectado (preservado)");

            if (g_list) {
                lv_obj_add_flag(g_list, LV_OBJ_FLAG_HIDDEN);
            }
            if (g_ssid_label) {
                lv_obj_add_flag(g_ssid_label, LV_OBJ_FLAG_HIDDEN);
            }
            if (g_password_ta) {
                lv_obj_add_flag(g_password_ta, LV_OBJ_FLAG_HIDDEN);
            }
            if (g_connect_btn) {
                lv_obj_add_flag(g_connect_btn, LV_OBJ_FLAG_HIDDEN);
            }
            hide_keyboard();
            
            wifi_enabled = false; // Interface desligada, mas Wi-Fi preservado
            return;
        }
        
        ui_set_status("Desligando Wi-Fi...");
        
        // Parar conexões ativas
        esp_wifi_disconnect();
        esp_wifi_stop();
        wifi_sta_started = false;
        
        // Desregistrar handlers
        if (wifi_events_registered) {
            esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler);
            esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler);
            wifi_events_registered = false;
        }

        if (wifi_service_initialized) {
            esp_wifi_deinit();
            wifi_service_initialized = false;
        }
        
        // Limpar interface
        if (g_list) {
            lv_obj_add_flag(g_list, LV_OBJ_FLAG_HIDDEN);
        }
        if (g_ssid_label) {
            lv_obj_add_flag(g_ssid_label, LV_OBJ_FLAG_HIDDEN);
        }
        if (g_password_ta) {
            lv_obj_add_flag(g_password_ta, LV_OBJ_FLAG_HIDDEN);
        }
        if (g_connect_btn) {
            lv_obj_add_flag(g_connect_btn, LV_OBJ_FLAG_HIDDEN);
        }
        hide_keyboard();
        
        // Limpar dados
        g_scan_count = 0;
        memset(g_selected_ssid, 0, sizeof(g_selected_ssid));
        wifi_connected = false;
        wifi_keep_alive = false;
        
        ui_set_status("Wi-Fi desligado");
        ESP_LOGI(TAG, "Wi-Fi desligado com sucesso");
    }
}

static void scan_and_list_networks(lv_obj_t *parent)
{
    if (!parent) {
        return;
    }

    if (!lvgl_port_lock(50)) {
        ESP_LOGW(TAG, "Não foi possível obter lock LVGL para atualizar lista de redes");
        return;
    }

    if (!lv_obj_is_valid(parent)) {
        lvgl_port_unlock();
        return;
    }

    lv_obj_clean(parent);
    
    for (int i = 0; i < g_scan_count; i++) {
        char display_text[48];
        snprintf(display_text, sizeof(display_text), "%s (%d)", 
                (char*)g_scan_records[i].ssid, g_scan_records[i].rssi);
        
        lv_obj_t *btn = lv_list_add_btn(parent, LV_SYMBOL_WIFI, display_text);
        lv_obj_add_event_cb(btn, wifi_list_event_cb, LV_EVENT_CLICKED, (void*)g_scan_records[i].ssid);
    }
    
    if (g_scan_count == 0) {
        lv_list_add_btn(parent, LV_SYMBOL_WARNING, "Nenhuma rede");
    }

    lvgl_port_unlock();
}

static void wifi_list_event_cb(lv_event_t *e)
{
    if (!wifi_has_ui()) {
        return;
    }

    const char *ssid = (const char *)lv_event_get_user_data(e);
    if (!ssid || strlen(ssid) == 0) {
        return;
    }

    ESP_LOGI(TAG, "Rede selecionada: %s", ssid);

    strncpy(g_selected_ssid, ssid, sizeof(g_selected_ssid) - 1);
    g_selected_ssid[sizeof(g_selected_ssid) - 1] = '\0';

    if (lvgl_port_lock(50)) {
        if (g_ssid_label && lv_obj_is_valid(g_ssid_label)) {
            lv_label_set_text_fmt(g_ssid_label, "Rede: %s", g_selected_ssid);
            lv_obj_clear_flag(g_ssid_label, LV_OBJ_FLAG_HIDDEN);
        }
        if (g_password_ta && lv_obj_is_valid(g_password_ta)) {
            lv_obj_clear_flag(g_password_ta, LV_OBJ_FLAG_HIDDEN);
        }
        if (g_connect_btn && lv_obj_is_valid(g_connect_btn)) {
            lv_obj_clear_flag(g_connect_btn, LV_OBJ_FLAG_HIDDEN);
        }
        lvgl_port_unlock();
    }

    bool has_saved_for_ssid = g_has_saved_credentials && (strcmp(g_selected_ssid, g_saved_ssid) == 0);

    if (has_saved_for_ssid) {
        if (lvgl_port_lock(50)) {
            if (g_password_ta && lv_obj_is_valid(g_password_ta)) {
                lv_textarea_set_text(g_password_ta, g_saved_password);
            }
            lvgl_port_unlock();
        }

        ui_set_status_fmt("Reconectando automaticamente a %s", g_selected_ssid);
        hide_keyboard();

        const char *password_to_use = g_saved_password[0] ? g_saved_password : "";
        esp_err_t ret = wifi_connect_internal(g_selected_ssid, password_to_use, true);
        if (ret != ESP_OK) {
            ui_set_status("Falha ao iniciar reconexão automática");
            show_keyboard(g_password_ta);
        }
    } else {
        if (lvgl_port_lock(50)) {
            if (g_password_ta && lv_obj_is_valid(g_password_ta)) {
                lv_textarea_set_text(g_password_ta, "");
            }
            lvgl_port_unlock();
        }

        ui_set_status_fmt("Digite a senha para: %s", g_selected_ssid);
        show_keyboard(g_password_ta);

        ESP_LOGI(TAG, "Interface de senha mostrada para: %s", g_selected_ssid);
    }
}

static void password_event_cb(lv_event_t *e)
{
    (void)e;
    if (!wifi_has_ui()) {
        return;
    }

    show_keyboard(g_password_ta);
}

static void show_keyboard(lv_obj_t *ta)
{
    if (!wifi_has_ui()) {
        return;
    }

    if (!lvgl_port_lock(50)) {
        ESP_LOGW(TAG, "Não foi possível obter lock LVGL para mostrar teclado");
        return;
    }

    if (!g_keyboard && g_screen && lv_obj_is_valid(g_screen)) {
        ESP_LOGI(TAG, "Criando teclado");
        g_keyboard = lv_keyboard_create(g_screen);
        lv_keyboard_set_textarea(g_keyboard, ta);
        lv_obj_set_size(g_keyboard, 480, 150);
        lv_obj_align(g_keyboard, LV_ALIGN_TOP_MID, 0, 300);
        lv_obj_add_event_cb(g_keyboard, keyboard_event_cb, LV_EVENT_ALL, NULL);
        ESP_LOGI(TAG, "Teclado criado e posicionado");
    } else if (g_keyboard && lv_obj_is_valid(g_keyboard)) {
        ESP_LOGI(TAG, "Mostrando teclado existente");
        lv_obj_clear_flag(g_keyboard, LV_OBJ_FLAG_HIDDEN);
        lv_keyboard_set_textarea(g_keyboard, ta);
    }

    lvgl_port_unlock();
}

static void hide_keyboard(void)
{
    if (!wifi_has_ui() || !g_keyboard || !lv_obj_is_valid(g_keyboard)) {
        return;
    }

    if (lvgl_port_lock(50)) {
        ESP_LOGI(TAG, "Escondendo teclado");
        lv_obj_add_flag(g_keyboard, LV_OBJ_FLAG_HIDDEN);
        lvgl_port_unlock();
    }
}

static void keyboard_event_cb(lv_event_t *e)
{
    if (!wifi_has_ui()) {
        return;
    }

    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        ESP_LOGI(TAG, "Teclado fechado pelo usuário");
        hide_keyboard();
        
        // Se foi READY (Enter), focar no botão conectar
        if (code == LV_EVENT_READY && g_connect_btn && !lv_obj_has_flag(g_connect_btn, LV_OBJ_FLAG_HIDDEN)) {
            ESP_LOGI(TAG, "Senha inserida, pronto para conectar");
        }
    }
}

static void wifi_connect_event_cb(lv_event_t *e)
{
    (void)e;
    
    if (strlen(g_selected_ssid) == 0) {
        ui_set_status("Erro: Nenhuma rede selecionada");
        ESP_LOGE(TAG, "Tentativa de conexão sem rede selecionada");
        return;
    }
    
    const char *password = lv_textarea_get_text(g_password_ta);
    
    ESP_LOGI(TAG, "Iniciando conexão com: %s", g_selected_ssid);
    
    ui_set_status_fmt("Conectando a %s...", g_selected_ssid);
    hide_keyboard();
    esp_err_t ret = wifi_connect_internal(g_selected_ssid, password, true);
    if (ret != ESP_OK) {
        ui_set_status("Falha ao conectar");
    }
}

static esp_err_t wifi_connect_internal(const char *ssid, const char *password, bool triggered_by_ui)
{
    if (!ssid || strlen(ssid) == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = wifi_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Não foi possível inicializar Wi-Fi antes de conectar");
        return ret;
    }

    ESP_LOGI(TAG, "Configurando Wi-Fi: SSID=%s, Password_len=%d", ssid, password ? strlen(password) : 0);

    strncpy(g_selected_ssid, ssid, sizeof(g_selected_ssid) - 1);
    g_selected_ssid[sizeof(g_selected_ssid) - 1] = '\0';

    if (password) {
        strncpy(g_current_password, password, sizeof(g_current_password) - 1);
        g_current_password[sizeof(g_current_password) - 1] = '\0';
    } else {
        g_current_password[0] = '\0';
    }

    if (triggered_by_ui && g_ssid_label && wifi_has_ui() && lvgl_port_lock(50)) {
        lv_label_set_text_fmt(g_ssid_label, "Rede: %s", g_selected_ssid);
        lv_obj_clear_flag(g_ssid_label, LV_OBJ_FLAG_HIDDEN);
        lvgl_port_unlock();
    }

    wifi_config_t wifi_config = {0};
    strncpy((char*)wifi_config.sta.ssid, g_selected_ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, g_current_password, sizeof(wifi_config.sta.password) - 1);

    wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
    wifi_config.sta.scan_method = WIFI_FAST_SCAN;
    wifi_config.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;

    esp_wifi_disconnect();
    vTaskDelay(pdMS_TO_TICKS(50));

    ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao configurar Wi-Fi: %s", esp_err_to_name(ret));
        if (triggered_by_ui) {
            ui_set_status("Erro na configuração");
        }
        return ret;
    }

    wifi_keep_alive = true;
    wifi_connected = false;

    ret = esp_wifi_connect();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao iniciar conexão: %s", esp_err_to_name(ret));
        if (triggered_by_ui) {
            ui_set_status("Falha ao conectar");
        }
        return ret;
    }

    g_last_reconnect_attempt = xTaskGetTickCount();
    ESP_LOGI(TAG, "Comando de conexão enviado com sucesso");
    return ESP_OK;
}

static void back_button_event_cb(lv_event_t *e)
{
    (void)e;
    
    // Preservar conexão Wi-Fi se estiver conectado
    wifi_main_preserve_connection();
    
    if (g_back_callback) {
        g_back_callback();
    }

    wifi_detach_ui();
}

void wifi_main_set_back_callback(wifi_back_cb_t callback)
{
    g_back_callback = callback;
}

static esp_err_t wifi_start_stack(void)
{
    if (wifi_service_initialized) {
        return ESP_OK;
    }

    esp_err_t ret = esp_netif_init();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Falha ao inicializar netif: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Falha ao criar event loop: %s", esp_err_to_name(ret));
        return ret;
    }

    if (!g_sta_netif) {
        g_sta_netif = esp_netif_create_default_wifi_sta();
        if (!g_sta_netif) {
            ESP_LOGE(TAG, "Falha ao criar STA netif");
            return ESP_FAIL;
        }
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK && ret != ESP_ERR_WIFI_INIT_STATE) {
        ESP_LOGE(TAG, "Falha ao inicializar Wi-Fi: %s", esp_err_to_name(ret));
        return ret;
    }

    wifi_service_initialized = true;
    return ESP_OK;
}

static esp_err_t wifi_init(void)
{
    esp_err_t ret = wifi_start_stack();
    if (ret != ESP_OK) {
        ui_set_status("Erro ao iniciar Wi-Fi");
        return ret;
    }

    if (!wifi_events_registered) {
        ret = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Falha ao registrar WIFI handler: %s", esp_err_to_name(ret));
            ui_set_status("Erro: handler Wi-Fi");
            return ret;
        }

        ret = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Falha ao registrar IP handler: %s", esp_err_to_name(ret));
            ui_set_status("Erro: handler IP");
            return ret;
        }

        wifi_events_registered = true;
    }

    if (!wifi_sta_started) {
        ret = esp_wifi_set_mode(WIFI_MODE_STA);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Falha ao definir modo STA: %s", esp_err_to_name(ret));
            ui_set_status("Erro: modo STA");
            return ret;
        }

        ret = esp_wifi_start();
        if (ret != ESP_OK && ret != ESP_ERR_WIFI_CONN) {
            ESP_LOGE(TAG, "Falha ao iniciar Wi-Fi: %s", esp_err_to_name(ret));
            ui_set_status("Erro: start Wi-Fi");
            return ret;
        }

        esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
        wifi_sta_started = true;
        ESP_LOGI(TAG, "Wi-Fi STA iniciado");
    }

    wifi_enabled = true;
    ui_set_status("Wi-Fi iniciado");
    return ESP_OK;
}

static void wifi_scan_start(void)
{
    ui_set_status("Escaneando redes...");
    
    // Limpar scan anterior
    g_scan_count = 0;
    
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = false,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time.active = {
            .min = 100,
            .max = 300
        }
    };
    
    esp_err_t ret = esp_wifi_scan_start(&scan_config, false);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao iniciar scan: %s", esp_err_to_name(ret));
        ui_set_status("Erro no scan");
        
        if (g_list && lvgl_port_lock(50)) {
            if (lv_obj_is_valid(g_list)) {
                lv_obj_clean(g_list);
                lv_list_add_btn(g_list, LV_SYMBOL_WARNING, "Erro no scan");
            }
            lvgl_port_unlock();
        }
    }
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_SCAN_DONE:
                ESP_LOGI(TAG, "Scan concluído");
                g_scan_count = MAX_SCAN_RECORDS;
                esp_err_t ret = esp_wifi_scan_get_ap_records(&g_scan_count, g_scan_records);
                if (ret == ESP_OK) {
                    ESP_LOGI(TAG, "Encontradas %d redes", g_scan_count);
                    ui_set_status_fmt("Encontradas: %d redes", g_scan_count);
                    scan_and_list_networks(g_list);
                } else {
                    ESP_LOGE(TAG, "Erro ao obter registros do scan: %s", esp_err_to_name(ret));
                    ui_set_status("Erro ao listar redes");
                    if (g_list && lvgl_port_lock(50)) {
                        if (lv_obj_is_valid(g_list)) {
                            lv_obj_clean(g_list);
                            lv_list_add_btn(g_list, LV_SYMBOL_WARNING, "Erro nos resultados");
                        }
                        lvgl_port_unlock();
                    }
                    g_scan_count = 0;
                }
                break;
                
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "Wi-Fi STA iniciado");
                break;
                
            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG, "Conectado ao Wi-Fi: %s", g_selected_ssid);
                wifi_connected = true;
                ui_set_status_fmt("Conectado a %s - Obtendo IP...", g_selected_ssid);
                break;
                
            case WIFI_EVENT_STA_DISCONNECTED:
                {
                    wifi_event_sta_disconnected_t* disconnected = (wifi_event_sta_disconnected_t*) event_data;
                    ESP_LOGI(TAG, "Desconectado do Wi-Fi. Motivo: %d", disconnected->reason);
                    wifi_connected = false;
                    
                    // Tratar diferentes motivos de desconexão
                    switch (disconnected->reason) {
                        case WIFI_REASON_AUTH_EXPIRE:
                        case WIFI_REASON_AUTH_FAIL:
                        case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
                            ui_set_status("Erro: Senha incorreta");
                            ESP_LOGE(TAG, "Erro de autenticação");
                            wifi_clear_credentials();
                            wifi_keep_alive = false;
                            break;
                        case WIFI_REASON_NO_AP_FOUND:
                            ui_set_status("Erro: Rede não encontrada");
                            ESP_LOGE(TAG, "Rede não encontrada");
                            break;
                        case WIFI_REASON_ASSOC_LEAVE:
                            ui_set_status("Desconectado pelo usuário");
                            break;
                        default:
                            ui_set_status_fmt("Falha na conexão (motivo: %d)", disconnected->reason);
                            ESP_LOGE(TAG, "Falha de conexão, motivo: %d", disconnected->reason);
                            break;
                    }

                    if (g_has_saved_credentials && wifi_keep_alive) {
                        TickType_t now = xTaskGetTickCount();
                        if ((now - g_last_reconnect_attempt) >= pdMS_TO_TICKS(WIFI_RECONNECT_DELAY_MS)) {
                            ESP_LOGI(TAG, "Tentando reconectar automaticamente ao Wi-Fi salvo");
                            esp_wifi_connect();
                            g_last_reconnect_attempt = now;
                        }
                    }
                }
                break;
                
            case WIFI_EVENT_STA_STOP:
                ESP_LOGI(TAG, "Wi-Fi STA parado");
                break;
                
            default:
                ESP_LOGI(TAG, "Evento Wi-Fi não tratado: %ld", event_id);
                break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "IP obtido: " IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "Gateway: " IPSTR, IP2STR(&event->ip_info.gw));
        ESP_LOGI(TAG, "Netmask: " IPSTR, IP2STR(&event->ip_info.netmask));
        
        // Configurar DNS para melhorar conectividade
        esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
        if (netif) {
            esp_netif_dns_info_t dns_info;
            
            // DNS primário: Google DNS 8.8.8.8
            IP4_ADDR(&dns_info.ip.u_addr.ip4, 8, 8, 8, 8);
            dns_info.ip.type = ESP_IPADDR_TYPE_V4;
            esp_netif_set_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns_info);
            
            // DNS secundário: Cloudflare DNS 1.1.1.1
            IP4_ADDR(&dns_info.ip.u_addr.ip4, 1, 1, 1, 1);
            esp_netif_set_dns_info(netif, ESP_NETIF_DNS_BACKUP, &dns_info);
            
            ESP_LOGI(TAG, "DNS configurado: 8.8.8.8 e 1.1.1.1");
        }
        
        wifi_connected = true;
        wifi_keep_alive = true;  // Marcar para preservar conexão
        ui_set_status_fmt("Conectado - IP: " IPSTR, IP2STR(&event->ip_info.ip));

        if (g_selected_ssid[0] != '\0') {
            wifi_save_credentials(g_selected_ssid, g_current_password);
        }

        if (wifi_has_ui() && lvgl_port_lock(50)) {
            if (g_switch && lv_obj_is_valid(g_switch)) {
                lv_obj_add_state(g_switch, LV_STATE_CHECKED);
            }

            if (g_ssid_label && lv_obj_is_valid(g_ssid_label) && g_selected_ssid[0] != '\0') {
                lv_label_set_text_fmt(g_ssid_label, "Rede: %s", g_selected_ssid);
                lv_obj_clear_flag(g_ssid_label, LV_OBJ_FLAG_HIDDEN);
            }

            lvgl_port_unlock();
        }
    }
}

// Funções para preservar Wi-Fi
void wifi_main_preserve_connection(void)
{
    wifi_enabled = true;
    wifi_keep_alive = true;

    if (!wifi_service_initialized) {
        (void)wifi_init();
    }

    if (!wifi_connected && g_has_saved_credentials) {
        TickType_t now = xTaskGetTickCount();
        if ((now - g_last_reconnect_attempt) >= pdMS_TO_TICKS(WIFI_RECONNECT_DELAY_MS)) {
            ESP_LOGI(TAG, "Forçando tentativa de reconexão preservada");
            esp_wifi_connect();
            g_last_reconnect_attempt = now;
        }
    }
}

void wifi_main_restore_connection(void)
{
    if (!wifi_has_ui()) {
        return;
    }

    wifi_enabled = true;

    if (!wifi_service_initialized) {
        wifi_init();
    }

    if (g_switch && lv_obj_is_valid(g_switch)) {
        lv_obj_add_state(g_switch, LV_STATE_CHECKED);
    }

    if (wifi_connected) {
        ESP_LOGI(TAG, "Restaurando estado da conexão Wi-Fi");
        if (g_status_label && lv_obj_is_valid(g_status_label)) {
            esp_netif_t *netif = g_sta_netif ? g_sta_netif : esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
            if (netif) {
                esp_netif_ip_info_t ip_info;
                if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK && ip_info.ip.addr != 0) {
                    ui_set_status_fmt("Conectado - IP: " IPSTR, IP2STR(&ip_info.ip));
                } else if (g_selected_ssid[0] != '\0') {
                    ui_set_status_fmt("Conectado a %s", g_selected_ssid);
                } else {
                    ui_set_status("Wi-Fi conectado");
                }
            }
        }
    } else {
        if (wifi_service_initialized) {
            ui_set_status("Wi-Fi ativo");
        }

        if (g_has_saved_credentials && g_ssid_label && lv_obj_is_valid(g_ssid_label)) {
            lv_label_set_text_fmt(g_ssid_label, "Rede salva: %s", g_selected_ssid);
            lv_obj_clear_flag(g_ssid_label, LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (g_list && lv_obj_is_valid(g_list) && wifi_sta_started) {
        lv_obj_clear_flag(g_list, LV_OBJ_FLAG_HIDDEN);
        wifi_scan_start();
    }
}

bool wifi_main_is_connected(void)
{
    return wifi_connected;
}

void wifi_main_service_init(void)
{
    if (wifi_service_started) {
        ESP_LOGI(TAG, "Serviço Wi-Fi já inicializado");
        return;
    }

    ESP_LOGI(TAG, "Inicializando serviço Wi-Fi em background");

    wifi_keep_alive = true;
    wifi_enabled = true;

    if (wifi_init() != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao iniciar serviço Wi-Fi");
        return;
    }

    wifi_service_started = true;

    char ssid[sizeof(g_selected_ssid)] = {0};
    char password[sizeof(g_current_password)] = {0};

    if (wifi_load_credentials(ssid, sizeof(ssid), password, sizeof(password))) {
        ESP_LOGI(TAG, "Reconectando automaticamente à rede salva: %s", ssid);
        if (wifi_connect_internal(ssid, password, false) != ESP_OK) {
            ESP_LOGW(TAG, "Falha ao iniciar reconexão automática. Será necessário tentar manualmente.");
        }
    } else {
        ESP_LOGI(TAG, "Nenhuma credencial Wi-Fi salva. Aguardando configuração.");
    }
}
