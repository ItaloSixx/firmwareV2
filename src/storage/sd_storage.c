#include "sd_storage.h"
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "driver/gpio.h"

#include "../config.h"

static const char *TAG_SD = "SD_STORAGE";
static sdmmc_card_t *s_card = NULL;
static bool s_mounted = false;

bool sd_storage_is_mounted(void) {
    return s_mounted;
}

bool sd_storage_init(void)
{
    if (s_mounted) {
        ESP_LOGI(TAG_SD, "SD já montado em %s", SD_MOUNT_POINT);
        return true;
    }

    // Configurar host SDMMC
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SD_MAX_FREQ_KHZ; // ~20 MHz por padrão

    // Configurar pinos e largura do barramento
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.clk = SD_PIN_CLK;
    slot_config.cmd = SD_PIN_CMD;
    slot_config.d0  = SD_PIN_D0;
    slot_config.width = SD_BUS_WIDTH; // 1 ou 4

    if (SD_BUS_WIDTH == 4) {
        slot_config.d1 = SD_PIN_D1;
        slot_config.d2 = SD_PIN_D2;
        slot_config.d3 = SD_PIN_D3;
    }

    // Opções de montagem VFS FAT
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 0,
        .disk_status_check_enable = true,
    };

    esp_err_t ret = esp_vfs_fat_sdmmc_mount(SD_MOUNT_POINT, &host, &slot_config, &mount_config, &s_card);
    if (ret == ESP_OK) {
        s_mounted = true;
        sdmmc_card_print_info(stdout, s_card);
        ESP_LOGI(TAG_SD, "SD montado em %s", SD_MOUNT_POINT);
        return true;
    }

    ESP_LOGE(TAG_SD, "Falha ao montar SD: %s", esp_err_to_name(ret));
    s_mounted = false;
    return false;
}

static bool file_exists_and_empty(const char *path, bool *is_empty)
{
    struct stat st;
    if (stat(path, &st) != 0) {
        // não existe
        *is_empty = true;
        return false;
    }
    *is_empty = (st.st_size == 0);
    return true;
}

esp_err_t sd_storage_append_csv(const char *filepath, const char *header, const char *line)
{
    if (!s_mounted) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!filepath || !line) {
        return ESP_ERR_INVALID_ARG;
    }

    // Garante que diretórios intermediários existam? (simples: assume raiz)
    FILE *f = fopen(filepath, "a+");
    if (!f) {
        ESP_LOGE(TAG_SD, "Erro abrindo %s: errno=%d", filepath, errno);
        return ESP_FAIL;
    }

    // Se o arquivo foi recém-criado, escreva cabeçalho
    long pos = ftell(f);
    fseek(f, 0, SEEK_END);
    long end = ftell(f);
    bool empty = (end == 0);

    if (empty && header && header[0] != '\0') {
        fprintf(f, "%s\n", header);
    }

    // Escreve linha de dados
    fprintf(f, "%s\n", line);
    fclose(f);
    return ESP_OK;
}
