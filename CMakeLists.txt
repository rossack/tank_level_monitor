# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 1.5.1)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)
set(PICO_USB_DEFAULT_CRITICAL_SECTION_TYPE spinlock) # Use "poll" architecture

project(tank_level_monitor C CXX ASM)
set(PICO_BOARD pico_w)
set(PROGRAM_NAME tank_level_monitor)
set(BUILD_VERSION 0.2.0)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()
#pico_set_program_version(${PROGRAM_NAME} ${BUILD_VERSION})

message("Running makefsdata python script")
execute_process(COMMAND
    python makefsdata.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)

add_definitions(-DBUILD_VERSION="${BUILD_VERSION}")
add_definitions(-DPICO_CYW43_ARCH_POLL)
add_definitions(-DDEBUG_BUILD)

# Add executable. Default name is the project name

add_executable(${PROGRAM_NAME}
    monitor.cpp
)


# Add the standard include files to the build
target_include_directories(${PROGRAM_NAME} PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/httpserver
  ${CMAKE_CURRENT_LIST_DIR}/dhcpserver
)

# Add additional source files
target_sources(${PROGRAM_NAME} PRIVATE
    tiny-json.c
    flash_memory.c
    mqtt_client.c
    utils.c
    button.cpp
    settings.c
    wifi.cpp
    ADCLogger.cpp
    httpserver/httpserver.c
    dhcpserver/dhcpserver.c
)


target_link_libraries(${PROGRAM_NAME}
    pico_stdlib
    #pico_cyw43_arch_lwip_threadsafe_background
    pico_cyw43_arch_lwip_poll
    pico_lwip_mqtt
    pico_lwip_http
    hardware_adc
    hardware_flash
    hardware_sync
)


#
# Note: Dont enable USB (PicoW's USB serial uses the same hardware as the Wi-Fi chip)
#
pico_enable_stdio_usb(${PROGRAM_NAME} 0)
pico_enable_stdio_uart(${PROGRAM_NAME} 1)

pico_add_extra_outputs(${PROGRAM_NAME})

