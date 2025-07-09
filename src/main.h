#ifndef MAIN_H
#define MAIN_H

#include "app/config.h"
#include "app/ui.h"
#include "app/tasks.h"

// Função principal do ESP-IDF
void app_main(void);

// Função de setup (compatibilidade)
void setup(void);

// Função loop (compatibilidade)  
void loop(void);

#endif // MAIN_H