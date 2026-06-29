cmake_minimum_required(VERSION 3.13)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(nerd_pico2_bsp_retired NONE)
    message(FATAL_ERROR
        "The standalone Pico2 BSP entry is retired. "
        "Configure from the repository root with -DBOARD=pico2/rp2350a/m33 "
        "or -DBOARD=pico2/rp2350a/hazard3."
    )
endif()

if(NOT NERD_TARGET)
    set(NERD_TARGET nerd)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmake/check_config.cmake)
include(${PROJECT_ROOT}/cmake/nerd_arch.cmake)
include(${PROJECT_ROOT}/cmake/nerd_sources.cmake)
include(${PROJECT_ROOT}/cmake/nerd_build_metadata.cmake)

if(NOT NERD_SOC_SOURCES OR NOT NERD_SAMPLE_SOURCES OR NOT NERD_BOARD_SOURCES)
    message(FATAL_ERROR
        "Pico2 legacy BSP fallback sources are retired. "
        "Configure from the repository root with -DBOARD=pico2/rp2350a/m33 "
        "or -DBOARD=pico2/rp2350a/hazard3 so arch/soc/board/sample layers export sources."
    )
endif()

if(NOT NERD_STARTUP_BACKEND)
    set(NERD_STARTUP_BACKEND legacy_bsp)
endif()
if(NOT NERD_STARTUP_OWNER)
    set(NERD_STARTUP_OWNER board)
endif()
if(NOT NERD_ENTRY_SOURCE)
    set(NERD_ENTRY_SOURCE ${CMAKE_CURRENT_LIST_DIR}/main.c)
endif()

set(NERD_BSP_SOURCES
    ${NERD_ENTRY_SOURCE}
)

add_executable(${NERD_TARGET}
    ${NERD_BSP_SOURCES}
    ${NERD_SAMPLE_SOURCES}
    ${NERD_BOARD_SOURCES}
    ${NERD_SOC_SOURCES}
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
    ${NERD_SAMPLE_INCLUDE_DIRS}
    ${NERD_BOARD_INCLUDE_DIRS}
    ${NERD_SOC_INCLUDE_DIRS}
    ${NERD_ARCH_INCLUDE_DIRS}
    ${NERD_COMMON_INCLUDE_DIRS}
)


if(NOT NERD_TOOLCHAIN_BACKEND)
    set(NERD_TOOLCHAIN_BACKEND pico_sdk)
endif()
if(NOT NERD_PICO_SDK_SYSTEM_INCLUDE_DIRS)
    set(NERD_PICO_SDK_SYSTEM_INCLUDE_DIRS ${PICO_SDK_PATH}/src/rp2_common)
endif()
if(NOT NERD_PICO_PRINTF_IMPLEMENTATION)
    set(NERD_PICO_PRINTF_IMPLEMENTATION pico)
endif()
if(NOT DEFINED NERD_PICO_EXTRA_OUTPUTS)
    set(NERD_PICO_EXTRA_OUTPUTS ON)
endif()
if(NOT NERD_PICO_LIBRARIES)
    set(NERD_PICO_LIBRARIES pico_stdlib)
endif()

target_include_directories(${NERD_TARGET} SYSTEM PRIVATE
    ${NERD_PICO_SDK_SYSTEM_INCLUDE_DIRS}
)

pico_set_printf_implementation(${NERD_TARGET} ${NERD_PICO_PRINTF_IMPLEMENTATION})

if(NERD_PICO_EXTRA_OUTPUTS)
    pico_add_extra_outputs(${NERD_TARGET})
endif()

if(NOT NERD_LINKER_SCRIPT)
    set(NERD_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/nerd.ld)
endif()
pico_set_linker_script(${NERD_TARGET} ${NERD_LINKER_SCRIPT})

foreach(linker_include_dir ${NERD_LINKER_INCLUDE_DIRS})
    target_link_options(${NERD_TARGET} PRIVATE "-Wl,-L${linker_include_dir}")
endforeach()

target_link_options(${NERD_TARGET} PRIVATE -Os
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.map
    -Wl,--cref
    -Wl,--print-memory-usage
)

target_link_libraries(${NERD_TARGET} ${NERD_PICO_LIBRARIES})
nerd_write_build_metadata(${PROJECT_BINARY_DIR}/build_layers.txt)

