cmake_minimum_required(VERSION 3.20)
project(myrtos_sysbuild NONE)

include(ExternalProject)

set(APP_DIR "${CMAKE_CURRENT_LIST_DIR}/.." CACHE PATH "Path to the MyRTOS application/build root")
set(MYRTOS_DOMAIN_C_COMPILER "" CACHE FILEPATH "C compiler passed to each domain build")

option(SB_CONFIG_PICO2_DUAL_DOMAIN "Build both Pico 2 M33 and Hazard3 domains" ON)
option(SB_CONFIG_QEMU_A9_DOMAIN "Build qemu_a9 as an extra domain" OFF)
option(SB_CONFIG_NUCLEO_F429ZI_DOMAIN "Build nucleo_f429zi as an extra domain" OFF)

set(_domain_names "")
set(_domains_yaml "domains:\n")

function(myrtos_add_domain domain_name board_target)
    set(domain_binary_dir ${CMAKE_BINARY_DIR}/domains/${domain_name})
    set(domain_cmake_args
        -S ${APP_DIR}
        -B ${domain_binary_dir}
        -G ${CMAKE_GENERATOR}
        -DBOARD=${board_target}
    )

    if(MYRTOS_DOMAIN_C_COMPILER)
        list(APPEND domain_cmake_args -DCMAKE_C_COMPILER=${MYRTOS_DOMAIN_C_COMPILER})
    endif()

    ExternalProject_Add(${domain_name}
        SOURCE_DIR ${APP_DIR}
        BINARY_DIR ${domain_binary_dir}
        CONFIGURE_COMMAND ${CMAKE_COMMAND} ${domain_cmake_args}
        BUILD_COMMAND ${CMAKE_COMMAND} --build ${domain_binary_dir}
        INSTALL_COMMAND ""
        USES_TERMINAL_CONFIGURE TRUE
        USES_TERMINAL_BUILD TRUE
    )

    add_custom_target(${domain_name}_run
        COMMAND ${domain_binary_dir}/app/app.exe
        DEPENDS ${domain_name}
        USES_TERMINAL
    )

    add_custom_target(${domain_name}_menuconfig
        COMMAND ${CMAKE_COMMAND} --build ${domain_binary_dir} --target menuconfig
        DEPENDS ${domain_name}
        USES_TERMINAL
    )

    set(_domain_names ${_domain_names} ${domain_name} PARENT_SCOPE)
    string(APPEND _domains_yaml
        "  - name: ${domain_name}\n"
        "    board: ${board_target}\n"
        "    build_dir: ${domain_binary_dir}\n"
        "    app: ${domain_binary_dir}/app/app.exe\n"
        "    config: ${domain_binary_dir}/.config\n"
        "    autoconf: ${domain_binary_dir}/include/generated/autoconf.h\n"
        "    devicetree: ${domain_binary_dir}/include/generated/devicetree_generated.h\n"
        "    layers: ${domain_binary_dir}/build_layers.txt\n"
    )
    set(_domains_yaml "${_domains_yaml}" PARENT_SCOPE)
endfunction()

if(SB_CONFIG_PICO2_DUAL_DOMAIN)
    myrtos_add_domain(pico2_m33 "pico2/rp2350a/m33")
    myrtos_add_domain(pico2_hazard3 "pico2/rp2350a/hazard3")
endif()

if(SB_CONFIG_QEMU_A9_DOMAIN)
    myrtos_add_domain(qemu_a9 "qemu_a9")
endif()

if(SB_CONFIG_NUCLEO_F429ZI_DOMAIN)
    myrtos_add_domain(nucleo_f429zi "nucleo_f429zi")
endif()

if(NOT _domain_names)
    message(FATAL_ERROR "No sysbuild domains enabled")
endif()

add_custom_target(domains DEPENDS ${_domain_names})

file(WRITE ${CMAKE_BINARY_DIR}/domains.yaml "${_domains_yaml}")
file(WRITE ${CMAKE_BINARY_DIR}/domains.txt "")
foreach(domain_name ${_domain_names})
    file(APPEND ${CMAKE_BINARY_DIR}/domains.txt "${domain_name}\n")
endforeach()

message(STATUS "Sysbuild domains: ${_domain_names}")
message(STATUS "Sysbuild metadata: ${CMAKE_BINARY_DIR}/domains.yaml")