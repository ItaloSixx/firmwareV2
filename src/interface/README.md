# Interface Modular - JC3248W535EN

## 📁 Estrutura de Pastas

```
src/interface/
├── components/           # Componentes reutilizáveis
│   ├── ui_components.h   # Headers dos componentes
│   └── ui_components.c   # Implementação dos componentes
├── screens/              # Telas modulares
│   ├── screen_home.h     # Tela principal
│   ├── screen_home.c
│   ├── screen_sensors.h  # Tela de sensores
│   ├── screen_sensors.c
│   ├── screen_settings.h # Tela de configurações
│   ├── screen_settings.c
│   ├── screen_about.h    # Tela sobre
│   └── screen_about.c
├── styles/               # Estilos centralizados
│   ├── ui_styles.h       # Constantes e definições
│   └── ui_styles.c       # Implementação dos estilos
├── ui_manager.h          # Gerenciador principal
├── ui_manager.c          # Nova API modular
├── ui_os.h              # API de compatibilidade
├── ui_os.c              # Ponte para nova API
├── ui_demo.h            # Demonstração
└── ui_demo.c            # Funcionalidades de demo
```

## 🧩 Componentes Reutilizáveis

### Status Bar (Barra Superior)
- **Função**: `ui_create_status_bar()`
- **Recursos**: Horário, ícones de status (WiFi, Bluetooth, etc.)
- **Atualização**: `ui_update_status_time()`

### Navigation Bar (Barra Inferior)
- **Função**: `ui_create_nav_bar()`
- **Recursos**: 4 botões de navegação com ícones e labels
- **Callback**: Automático para troca de telas
- **Indicador**: Destaque da tela ativa

### Cards e Botões
- **Cards**: `ui_create_card()` - Containers com estilo Material Design
- **Botões Primários**: `ui_create_button_primary()` - Ação principal
- **Botões Secundários**: `ui_create_button_secondary()` - Ações auxiliares

### Textos
- **Títulos**: `ui_create_title()` - Cabeçalhos de seção
- **Corpo**: `ui_create_body_text()` - Texto normal

## 🎨 Sistema de Estilos

### Cores (Material Design Dark)
```c
#define UI_COLOR_PRIMARY           0x6200EE  // Roxo primário
#define UI_COLOR_SECONDARY         0x03DAC6  // Teal secundário
#define UI_COLOR_BACKGROUND        0x121212  // Fundo escuro
#define UI_COLOR_SURFACE           0x1E1E1E  // Superfície
```

### Dimensões
```c
#define UI_SCREEN_WIDTH            480       // Largura tela
#define UI_SCREEN_HEIGHT           320       // Altura tela
#define UI_STATUS_BAR_HEIGHT       30        // Barra superior
#define UI_NAV_BAR_HEIGHT          50        // Barra inferior
#define UI_CONTENT_HEIGHT          240       // Área útil
```

### Funções de Estilo
- `ui_apply_card_style()` - Estilo de cards
- `ui_apply_button_primary_style()` - Botões principais
- `ui_apply_title_style()` - Títulos
- `ui_apply_body_style()` - Texto corpo

## 📱 Telas Modulares

### 1. Home (Tela Principal)
- **Arquivo**: `screen_home.c`
- **Conteúdo**: Grid 2x2 com botões principais
- **Navegação**: Para todas as outras telas

### 2. Sensors (Monitoramento)
- **Arquivo**: `screen_sensors.c`
- **Conteúdo**: 
  - Card BNO055: Pitch, Roll, Yaw, Battery
  - Card Outros: LIDAR, Status da bateria
- **Atualização**: Tempo real via `screen_sensors_update()`

### 3. Settings (Configurações)
- **Arquivo**: `screen_settings.c`
- **Conteúdo**:
  - Configurações Básicas: Brilho, Volume, Data/Hora
  - Configurações Avançadas: WiFi, Calibração, Backup

### 4. About (Sobre)
- **Arquivo**: `screen_about.c`
- **Conteúdo**: Informações do hardware e software

## ⚙️ Gerenciador (ui_manager)

### Inicialização
```c
ui_manager_t *manager = ui_manager_init();
```

### Navegação
```c
ui_manager_set_screen(manager, UI_SCREEN_SENSORS);
ui_screen_t current = ui_manager_get_current_screen(manager);
```

### Atualizações
```c
ui_manager_update_sensor_data(manager, &sensor_data);
ui_manager_update_system_state(manager, &system_state);
ui_manager_update(manager); // Chamada periódica
```

## 🔄 API de Compatibilidade

Para manter compatibilidade com código existente:

```c
// Funções antigas continuam funcionando
ui_os_init();
ui_set_screen(UI_SCREEN_SENSORS);
ui_update_sensor_data(&data);
ui_os_update();
```

## 🚀 Vantagens da Modularização

### ✅ Benefícios
1. **Reutilização**: Componentes podem ser usados em múltiplas telas
2. **Manutenção**: Cada tela em arquivo separado
3. **Estilos Centralizados**: Mudanças visuais em um local
4. **Performance**: Apenas tela ativa é renderizada
5. **Escalabilidade**: Fácil adicionar novas telas

### 🎯 Melhorias Implementadas
- **Sem Recriação**: Telas são mantidas em memória
- **Navegação Suave**: Transições sem delay
- **Componentização**: Status bar e nav bar reutilizáveis
- **Tema Consistente**: Material Design aplicado
- **Compatibilidade**: API antiga mantida

## 📊 Uso de Memória

- **Status Bar**: ~2KB (criada uma vez)
- **Nav Bar**: ~3KB (criada uma vez)
- **Cada Tela**: ~4-8KB (criada sob demanda)
- **Gerenciador**: ~1KB (estrutura de controle)

**Total Estimado**: 15-25KB (muito menor que implementação anterior)

## 🔧 Como Adicionar Nova Tela

1. **Criar arquivos**:
   ```
   src/interface/screens/screen_nova.h
   src/interface/screens/screen_nova.c
   ```

2. **Adicionar enum**:
   ```c
   typedef enum {
       UI_SCREEN_HOME = 0,
       UI_SCREEN_SENSORS,
       UI_SCREEN_SETTINGS,
       UI_SCREEN_ABOUT,
       UI_SCREEN_NOVA,    // <- Nova tela
       UI_SCREEN_COUNT
   } ui_screen_t;
   ```

3. **Implementar funções**:
   ```c
   lv_obj_t *screen_nova_create(lv_obj_t *parent);
   void screen_nova_update(lv_obj_t *screen);
   void screen_nova_destroy(lv_obj_t *screen);
   ```

4. **Integrar no manager**:
   ```c
   case UI_SCREEN_NOVA:
       manager->screens[screen_type] = screen_nova_create(manager->content_area);
       break;
   ```

## 🎉 Demonstração

Use `ui_demo.c` para testar:

```c
// Inicializar demo
ui_demo_init();

// Criar task de atualização
xTaskCreate(ui_demo_update_task, "ui_demo", 4096, NULL, 5, NULL);

// Testar navegação
ui_demo_navigation_test();
```