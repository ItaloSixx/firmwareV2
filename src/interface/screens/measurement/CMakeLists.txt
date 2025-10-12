# CMakeLists.txt para o módulo de medição de plantas
# Adiciona os arquivos de medição ao build

set(MEASUREMENT_SOURCES
    "measurement_main.c"
)

# Adicionar ao COMPONENT_SRCS da pasta pai
set(COMPONENT_SRCS ${COMPONENT_SRCS} ${MEASUREMENT_SOURCES} PARENT_SCOPE)