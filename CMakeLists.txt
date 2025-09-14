cmake_minimum_required(VERSION 3.13.1)

set(BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/.)

if(NOT DEFINED BOARD)
    set(BOARD "nrf52840dk/nrf52840")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zephyr-link-tic)

set(app_sources
    src/main.c
    src/tic.c
)

target_include_directories(app PRIVATE include)
target_sources(app PRIVATE ${app_sources})
target_sources_ifdef(CONFIG_BT app PRIVATE src/ble.c)
target_sources_ifdef(CONFIG_TIC_DEMO_MODE app PRIVATE 
    src/tic_demo.c
    src/tic_demo_data.c
)