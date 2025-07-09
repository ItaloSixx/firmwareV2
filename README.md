# Estrutura da Aplicação

## Organização dos Arquivos

### Pasta `src/` (Raiz)
- **`main.c`** / **`main.h`** - Arquivo principal da aplicação (anteriormente DEMO_LVGL.c)
- **Arquivos BSP** - Board Support Package (não modificar):
  - `esp_bsp.c` / `esp_bsp.h` - Configurações da placa
  - `esp_lcd_*.c` / `esp_lcd_*.h` - Drivers do display LCD
  - `esp_lcd_touch.c` / `esp_lcd_touch.h` - Driver de touch
  - `lv_port.c` / `lv_port.h` - Portabilidade do LVGL
  - `display.h` - Definições do display
  - `lv_conf.h` - Configurações do LVGL
  - `bsp_err_check.h` - Macros de verificação de erro

### Pasta `src/app/` (Código da Aplicação)
- **`config.h`** - Configurações gerais da aplicação
- **`ui.c`** / **`ui.h`** - Interface do usuário LVGL
- **`tasks.c`** / **`tasks.h`** - Tarefas FreeRTOS

## Como Desenvolver

### 1. Configurações
Edite `app/config.h` para ajustar:
- Nome e versão da aplicação
- Configurações de display
- Configurações de tarefas
- Parâmetros gerais

### 2. Interface do Usuário
Edite `app/ui.c` para:
- Criar telas customizadas
- Implementar eventos de botões/touch
- Atualizar elementos visuais

### 3. Lógica da Aplicação
Edite `app/tasks.c` para:
- Implementar tarefas em background
- Processamento de dados
- Comunicação entre tarefas

### 4. Arquivo Principal
O `main.c` geralmente não precisa ser modificado, exceto para:
- Adicionar novas inicializações de hardware
- Modificar configurações de boot

## Compilação

```bash
# Compilar o projeto
pio run

# Upload para a placa
pio run --target upload

# Monitor serial
pio device monitor
```

## Estrutura de Tarefas

A aplicação usa o modelo de tarefas FreeRTOS:

1. **Tarefa UI Update** - Atualiza interface a cada 50ms
2. **Tarefa Main App** - Lógica principal da aplicação
3. **Tarefa LVGL** - Gerenciada automaticamente pelo BSP

## Pontos de Entrada

- `app_main()` - Entrada principal do ESP-IDF
- `setup()` - Inicialização do sistema
- `app_ui_init()` - Inicialização da interface
- `app_tasks_init()` - Criação das tarefas
