#include "wifi_main.h"
#include "lvgl.h"
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <nvs_flash.h>
#include <esp_log.h>
#include <string.h>
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <lwip/inet.h>
#include <lwip/ip_addr.h>

static const char *TAG = "WIFI_MAIN";

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

// Buffer para armazenar redes escaneadas
#define MAX_SCAN_RECORDS 20
static wifi_ap_record_t g_scan_records[MAX_SCAN_RECORDS];
static uint16_t g_scan_count = 0;
static char g_selected_ssid[33] = {0};

static void wifi_switch_event_cb(lv_event_t *e);
static void wifi_connect_event_cb(lv_event_t *e);
static void wifi_list_event_cb(lv_event_t *e);
static void password_event_cb(lv_event_t *e);
static void show_keyboard(lv_obj_t *ta);
static void hide_keyboard(void);
static void keyboard_event_cb(lv_event_t *e);
static void scan_and_list_networks(lv_obj_t *parent);
static void back_button_event_cb(lv_event_t *e);
static void wifi_init(void);
static void wifi_scan_start(void);
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

lv_obj_t *wifi_main_create(lv_obj_t *parent)
{
    g_screen = lv_obj_create(parent);
    lv_obj_set_size(g_screen, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(g_screen, lv_color_hex(0xF5F5F5), LV_PART_MAIN);
    lv_obj_set_style_border_width(g_screen, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(g_screen, 0, LV_PART_MAIN);

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
        lv_label_set_text(g_status_label, "Iniciando Wi-Fi...");
        wifi_init();
        lv_obj_clear_flag(g_list, LV_OBJ_FLAG_HIDDEN);
        // Aguardar um pouco antes do scan
        vTaskDelay(pdMS_TO_TICKS(500));
        wifi_scan_start();
    } else {
        ESP_LOGI(TAG, "Solicitação para desligar Wi-Fi...");
        
        // Se estiver conectado e preservando, não desligar
        if (wifi_connected && wifi_keep_alive) {
            ESP_LOGI(TAG, "Wi-Fi conectado será preservado");
            lv_label_set_text(g_status_label, "Wi-Fi conectado (preservado)");
            
            // Apenas esconder interface
            lv_obj_add_flag(g_list, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(g_ssid_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(g_password_ta, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(g_connect_btn, LV_OBJ_FLAG_HIDDEN);
            hide_keyboard();
            
            wifi_enabled = false; // Interface desligada, mas Wi-Fi preservado
            return;
        }
        
        lv_label_set_text(g_status_label, "Desligando Wi-Fi...");
        
        // Parar conexões ativas
        esp_wifi_disconnect();
        esp_wifi_stop();
        
        // Desregistrar handlers
        esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler);
        esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler);
        
        esp_wifi_deinit();
        
        // Limpar interface
        lv_obj_add_flag(g_list, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(g_ssid_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(g_password_ta, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(g_connect_btn, LV_OBJ_FLAG_HIDDEN);
        hide_keyboard();
        
        // Limpar dados
        g_scan_count = 0;
        memset(g_selected_ssid, 0, sizeof(g_selected_ssid));
        wifi_connected = false;
        wifi_keep_alive = false;
        
        lv_label_set_text(g_status_label, "Wi-Fi desligado");
        ESP_LOGI(TAG, "Wi-Fi desligado com sucesso");
    }
}

static void scan_and_list_networks(lv_obj_t *parent)
{
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
}

static void wifi_list_event_cb(lv_event_t *e)
{
    const char *ssid = (const char *)lv_event_get_user_data(e);
    if (ssid && strlen(ssid) > 0) {
        ESP_LOGI(TAG, "Rede selecionada: %s", ssid);
        
        strncpy(g_selected_ssid, ssid, sizeof(g_selected_ssid) - 1);
        g_selected_ssid[sizeof(g_selected_ssid) - 1] = '\0';
        
        // Mostrar elementos da interface de senha
        lv_label_set_text_fmt(g_ssid_label, "Rede: %s", g_selected_ssid);
        lv_obj_clear_flag(g_ssid_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(g_password_ta, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(g_connect_btn, LV_OBJ_FLAG_HIDDEN);
        
        // Limpar senha anterior e focar no campo
        lv_textarea_set_text(g_password_ta, "");
        
        // Atualizar status
        lv_label_set_text_fmt(g_status_label, "Digite a senha para: %s", g_selected_ssid);
        
        // Mostrar teclado automaticamente
        show_keyboard(g_password_ta);
        
        ESP_LOGI(TAG, "Interface de senha mostrada para: %s", g_selected_ssid);
    }
}

static void password_event_cb(lv_event_t *e)
{
    show_keyboard(g_password_ta);
}

static void show_keyboard(lv_obj_t *ta)
{
    if (!g_keyboard && g_screen) {
        ESP_LOGI(TAG, "Criando teclado");
        g_keyboard = lv_keyboard_create(g_screen);
        lv_keyboard_set_textarea(g_keyboard, ta);
        lv_obj_set_size(g_keyboard, 480, 150);
        lv_obj_align(g_keyboard, LV_ALIGN_TOP_MID, 0, 300);  // Posicionar abaixo do campo senha
        lv_obj_add_event_cb(g_keyboard, keyboard_event_cb, LV_EVENT_ALL, NULL);
        
        ESP_LOGI(TAG, "Teclado criado e posicionado");
        
    } else if (g_keyboard) {
        ESP_LOGI(TAG, "Mostrando teclado existente");
        lv_obj_clear_flag(g_keyboard, LV_OBJ_FLAG_HIDDEN);
        lv_keyboard_set_textarea(g_keyboard, ta);
    }
}

static void hide_keyboard(void)
{
    if (g_keyboard) {
        ESP_LOGI(TAG, "Escondendo teclado");
        lv_obj_add_flag(g_keyboard, LV_OBJ_FLAG_HIDDEN);
    }
}

static void keyboard_event_cb(lv_event_t *e)
{
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
        lv_label_set_text(g_status_label, "Erro: Nenhuma rede selecionada");
        ESP_LOGE(TAG, "Tentativa de conexão sem rede selecionada");
        return;
    }
    
    const char *password = lv_textarea_get_text(g_password_ta);
    
    ESP_LOGI(TAG, "Iniciando conexão com: %s", g_selected_ssid);
    
    lv_label_set_text_fmt(g_status_label, "Conectando a %s...", g_selected_ssid);
    hide_keyboard();
    
    // Desconectar primeiro se já estiver conectado
    esp_wifi_disconnect();
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Configurar Wi-Fi
    wifi_config_t wifi_config = {0};
    strncpy((char*)wifi_config.sta.ssid, g_selected_ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    
    // Configuração mais flexível de autenticação
    wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;  // Permitir qualquer tipo
    wifi_config.sta.scan_method = WIFI_FAST_SCAN;
    wifi_config.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
    
    ESP_LOGI(TAG, "Configurando Wi-Fi: SSID=%s, Password_len=%d", 
             g_selected_ssid, strlen(password));
    
    esp_err_t ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao configurar Wi-Fi: %s", esp_err_to_name(ret));
        lv_label_set_text(g_status_label, "Erro na configuração");
        return;
    }
    
    ret = esp_wifi_connect();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao iniciar conexão: %s", esp_err_to_name(ret));
        lv_label_set_text(g_status_label, "Falha ao conectar");
    } else {
        ESP_LOGI(TAG, "Comando de conexão enviado com sucesso");
    }
}

static void back_button_event_cb(lv_event_t *e)
{
    (void)e;
    
    // Preservar conexão Wi-Fi se estiver conectado
    wifi_main_preserve_connection();
    
    if (g_back_callback) {
        g_back_callback();
    }
}

void wifi_main_set_back_callback(wifi_back_cb_t callback)
{
    g_back_callback = callback;
}

static void wifi_init(void)
{
    static bool wifi_initialized = false;
    
    if (wifi_initialized) {
        ESP_LOGI(TAG, "Wi-Fi já inicializado, reiniciando...");
        esp_wifi_stop();
        esp_wifi_deinit();
        wifi_initialized = false;
    }
    
    esp_err_t ret;
    
    // Inicializar NVS se necessário
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Inicializar componentes de rede
    ret = esp_netif_init();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Falha ao inicializar netif: %s", esp_err_to_name(ret));
        lv_label_set_text(g_status_label, "Erro: netif");
        return;
    }
    
    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Falha ao criar event loop: %s", esp_err_to_name(ret));
        lv_label_set_text(g_status_label, "Erro: event loop");
        return;
    }
    
    static esp_netif_t *sta_netif = NULL;
    if (!sta_netif) {
        sta_netif = esp_netif_create_default_wifi_sta();
        if (!sta_netif) {
            ESP_LOGE(TAG, "Falha ao criar STA netif");
            lv_label_set_text(g_status_label, "Erro: STA netif");
            return;
        }
    }
    
    // Configuração Wi-Fi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao inicializar Wi-Fi: %s", esp_err_to_name(ret));
        lv_label_set_text(g_status_label, "Erro: init Wi-Fi");
        return;
    }
    
    // Registrar handlers
    ret = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao registrar WIFI handler: %s", esp_err_to_name(ret));
        lv_label_set_text(g_status_label, "Erro: handler Wi-Fi");
        return;
    }
    
    ret = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao registrar IP handler: %s", esp_err_to_name(ret));
        lv_label_set_text(g_status_label, "Erro: handler IP");
        return;
    }
    
    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao definir modo STA: %s", esp_err_to_name(ret));
        lv_label_set_text(g_status_label, "Erro: modo STA");
        return;
    }
    
    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao iniciar Wi-Fi: %s", esp_err_to_name(ret));
        lv_label_set_text(g_status_label, "Erro: start Wi-Fi");
        return;
    }
    
    wifi_initialized = true;
    ESP_LOGI(TAG, "Wi-Fi inicializado com sucesso");
    lv_label_set_text(g_status_label, "Wi-Fi iniciado");
}

static void wifi_scan_start(void)
{
    lv_label_set_text(g_status_label, "Escaneando redes...");
    
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
        lv_label_set_text(g_status_label, "Erro no scan");
        
        // Limpar lista em caso de erro
        lv_obj_clean(g_list);
        lv_list_add_btn(g_list, LV_SYMBOL_WARNING, "Erro no scan");
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
                    lv_label_set_text_fmt(g_status_label, "Encontradas: %d redes", g_scan_count);
                    scan_and_list_networks(g_list);
                } else {
                    ESP_LOGE(TAG, "Erro ao obter registros do scan: %s", esp_err_to_name(ret));
                    lv_label_set_text(g_status_label, "Erro ao listar redes");
                    lv_obj_clean(g_list);
                    lv_list_add_btn(g_list, LV_SYMBOL_WARNING, "Erro nos resultados");
                    g_scan_count = 0;
                }
                break;
                
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "Wi-Fi STA iniciado");
                break;
                
            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG, "Conectado ao Wi-Fi: %s", g_selected_ssid);
                wifi_connected = true;
                lv_label_set_text_fmt(g_status_label, "Conectado a %s - Obtendo IP...", g_selected_ssid);
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
                            lv_label_set_text(g_status_label, "Erro: Senha incorreta");
                            ESP_LOGE(TAG, "Erro de autenticação");
                            break;
                        case WIFI_REASON_NO_AP_FOUND:
                            lv_label_set_text(g_status_label, "Erro: Rede não encontrada");
                            ESP_LOGE(TAG, "Rede não encontrada");
                            break;
                        case WIFI_REASON_ASSOC_LEAVE:
                            lv_label_set_text(g_status_label, "Desconectado pelo usuário");
                            break;
                        default:
                            lv_label_set_text_fmt(g_status_label, "Falha na conexão (motivo: %d)", disconnected->reason);
                            ESP_LOGE(TAG, "Falha de conexão, motivo: %d", disconnected->reason);
                            break;
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
        lv_label_set_text_fmt(g_status_label, "Conectado - IP: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

// Funções para preservar Wi-Fi
void wifi_main_preserve_connection(void)
{
    if (wifi_connected && wifi_keep_alive) {
        ESP_LOGI(TAG, "Preservando conexão Wi-Fi ativa");
        // Não desligar o Wi-Fi se estiver conectado
        wifi_enabled = true;
    }
}

void wifi_main_restore_connection(void)
{
    if (wifi_keep_alive && wifi_connected) {
        ESP_LOGI(TAG, "Restaurando estado da conexão Wi-Fi");
        wifi_enabled = true;
        
        // Atualizar interface se os elementos existirem
        if (g_switch) {
            lv_obj_add_state(g_switch, LV_STATE_CHECKED);
        }
        if (g_status_label) {
            // Obter IP atual se ainda conectado
            esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
            if (netif) {
                esp_netif_ip_info_t ip_info;
                if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK && ip_info.ip.addr != 0) {
                    lv_label_set_text_fmt(g_status_label, "Conectado - IP: " IPSTR, IP2STR(&ip_info.ip));
                } else {
                    lv_label_set_text_fmt(g_status_label, "Conectado a %s", g_selected_ssid);
                }
            }
        }
    }
}

bool wifi_main_is_connected(void)
{
    return wifi_connected && wifi_keep_alive;
}
