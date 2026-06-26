cmake_minimum_required(VERSION 3.13)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    include(pico_sdk_import.cmake)
    set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

    project(nerd C CXX ASM)

    set(PROJECT_ROOT                ${CMAKE_CURRENT_LIST_DIR}/../..)
    list(APPEND CMAKE_MODULE_PATH   ${PROJECT_ROOT}/cmake)
    if(NOT BUILD_CONFIG)
        set(BUILD_CONFIG pico2)
    endif()

    include(kconfig)

    set(CMAKE_C_STANDARD 11)
    set(CMAKE_CXX_STANDARD 17)

    pico_sdk_init()
endif()

if(NOT NERD_TARGET)
    set(NERD_TARGET nerd)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmake/check_config.cmake)
include(${PROJECT_ROOT}/cmake/nerd_arch.cmake)
include(${PROJECT_ROOT}/cmake/nerd_sources.cmake)

set(NERD_BSP_SOURCES
    main.c
    nd_hrtimer.c
    nd_led.c
    shell_port.c
    app.c
)

add_executable(${NERD_TARGET}
    ${NERD_BSP_SOURCES}
    ${NERD_ARCH_SOURCES}
    ${NERD_KERNEL_SOURCES}
    ${NERD_LIB_SOURCES}
    ${NERD_COMPONENT_SOURCES}
)

target_compile_options(${NERD_TARGET} PRIVATE
    -Wall
    -Wextra
    -Wunused
    -Wshadow
    -Werror=return-type
    -Werror=vla
    -Werror=uninitialized
    -Werror=format-security
    -Wdouble-promotion
    -Wno-missing-field-initializers
    -Wno-unused-but-set-variable
    $<$<COMPILE_LANGUAGE:C>:
        -Werror=implicit-function-declaration
        -Werror=implicit-int
        -Werror=strict-prototypes
        -Wno-pointer-sign
    >
)

target_include_directories(${NERD_TARGET} PRIVATE
    ${AUTOCONF_DIR}
    ${CMAKE_CURRENT_LIST_DIR}
    ${NERD_ARCH_INCLUDE_DIRS}
    ${NERD_COMMON_INCLUDE_DIRS}
)

target_include_directories(${NERD_TARGET} SYSTEM PRIVATE
    ${PICO_SDK_PATH}/src/rp2_common
)

pico_set_printf_implementation(${NERD_TARGET} pico)

pico_add_extra_outputs(${NERD_TARGET})

pico_set_linker_script(${NERD_TARGET} ${CMAKE_CURRENT_SOURCE_DIR}/nerd.ld)

target_link_options(${NERD_TARGET} PRIVATE -Os
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.map
    -Wl,--cref
    -Wl,--print-memory-usage
)

target_link_libraries(${NERD_TARGET} pico_stdlib)

