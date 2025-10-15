#ifndef SD_STORAGE_H
#define SD_STORAGE_H

#include <stdbool.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

// Inicializa e monta o cartão SD em SD_MOUNT_POINT.
// Retorna true se montado com sucesso.
bool sd_storage_init(void);

// Indica se o cartão está montado.
bool sd_storage_is_mounted(void);

// Acrescenta uma linha CSV no caminho indicado (cria o arquivo se não existir).
// Opcionalmente escreve o cabeçalho se o arquivo estiver vazio.
esp_err_t sd_storage_append_csv(const char *filepath, const char *header, const char *line);

#ifdef __cplusplus
}
#endif

#endif // SD_STORAGE_H
