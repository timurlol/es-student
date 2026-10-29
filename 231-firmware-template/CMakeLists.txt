cmake_minimum_required(VERSION 3.15)

set(PICO_BOARD pico)
set(PICO_PLATFORM rp2040)

include(pico_sdk_import.cmake)

project(231_firmware_template)

pico_sdk_init()

add_compile_options(-Wall)

# libs лежит вне папки проекта: второй аргумент задаёт каталог сборки
add_subdirectory(../libs ${CMAKE_BINARY_DIR}/libs)

# --- Железо прибора: единственное место проекта с SDK ---
add_library(platform STATIC
    platform/platform.c
)
target_include_directories(platform PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(platform PRIVATE
    pico_stdlib
)

# --- Логика прибора: задачи и команды, без SDK ---
add_library(logic STATIC
    logic/api/api.c
    logic/api/api-callbacks.c
    logic/led-task/led-task.c
    logic/button-task/button-task.c
    logic/pi-task/pi-task.c
)
target_include_directories(logic PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/logic
)
target_link_libraries(logic
    PUBLIC
        stdio_text_protocol
    PRIVATE
        platform
        systime
        profiling
        device
        logging
)

# --- Прошивка ---
add_executable(${PROJECT_NAME}
    main.c
)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    platform
    logic
    profiling
    logging
    systime
)

target_compile_definitions(profiling PUBLIC
    PROFILING_ENABLED=1
)

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)
pico_add_extra_outputs(${PROJECT_NAME})