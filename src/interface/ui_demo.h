/**
 * @file ui_demo.h
 * @brief Header para demonstração da UI modular
 * @author ItaloSixx
 * @date 2025
 */

#ifndef UI_DEMO_H
#define UI_DEMO_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa a demonstração da UI
 */
void ui_demo_init(void);

/**
 * @brief Task para atualizar dados de demonstração
 */
void ui_demo_update_task(void *pvParameters);

/**
 * @brief Teste de navegação entre telas
 */
void ui_demo_navigation_test(void);

#ifdef __cplusplus
}
#endif

#endif // UI_DEMO_H