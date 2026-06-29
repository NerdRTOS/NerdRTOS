cmake_minimum_required(VERSION 3.20)
project(nerd_sysbuild NONE)

include(ExternalProject)

get_filename_component(NERD_APP_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(NERD_APP_DIR "${NERD_APP_DIR}" CACHE PATH "Path to the NerdRTOS application/build root")

if(NOT NERD_DOMAIN_PICO_SDK_PATH AND PICO_SDK_PATH)
    set(NERD_DOMAIN_PICO_SDK_PATH "${PICO_SDK_PATH}" CACHE PATH "Pico SDK path passed to Pico domains")
elseif(NOT NERD_DOMAIN_PICO_SDK_PATH AND DEFINED ENV{PICO_SDK_PATH})
    set(NERD_DOMAIN_PICO_SDK_PATH "$ENV{PICO_SDK_PATH}" CACHE PATH "Pico SDK path passed to Pico domains")
else()
    set(NERD_DOMAIN_PICO_SDK_PATH "${NERD_DOMAIN_PICO_SDK_PATH}" CACHE PATH "Pico SDK path passed to Pico domains")
endif()

option(SB_CONFIG_PICO2_M33_DOMAIN "Build the Pico 2 Cortex-M33 domain" ON)
option(SB_CONFIG_PICO2_HAZARD3_DOMAIN "Build the Pico 2 Hazard3 RISC-V domain" ON)

set(_nerd_domain_names "")
set(_nerd_domains_yaml "domains:\n")

function(nerd_add_domain domain_name board_target)
    set(domain_binary_dir "${CMAKE_BINARY_DIR}/domains/${domain_name}")
    set(domain_cmake_args
        -S "${NERD_APP_DIR}"
        -B "${domain_binary_dir}"
        -G "${CMAKE_GENERATOR}"
        -DBOARD=${board_target}
    )

    if(CMAKE_MAKE_PROGRAM)
        list(APPEND domain_cmake_args -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM})
    endif()

    if(NERD_DOMAIN_PICO_SDK_PATH)
        list(APPEND domain_cmake_args -DPICO_SDK_PATH=${NERD_DOMAIN_PICO_SDK_PATH})
    endif()

    ExternalProject_Add(${domain_name}
        SOURCE_DIR "${NERD_APP_DIR}"
        BINARY_DIR "${domain_binary_dir}"
        CONFIGURE_COMMAND ${CMAKE_COMMAND} ${domain_cmake_args}
        BUILD_COMMAND ${CMAKE_COMMAND} --build "${domain_binary_dir}"
        INSTALL_COMMAND ""
        STEP_TARGETS configure build
        USES_TERMINAL_CONFIGURE TRUE
        USES_TERMINAL_BUILD TRUE
    )

    add_custom_target(${domain_name}_menuconfig
        COMMAND ${CMAKE_COMMAND} --build "${domain_binary_dir}" --target menuconfig
        DEPENDS ${domain_name}-configure
        USES_TERMINAL
    )

    add_custom_target(${domain_name}_guiconfig
        COMMAND ${CMAKE_COMMAND} --build "${domain_binary_dir}" --target guiconfig
        DEPENDS ${domain_name}-configure
        USES_TERMINAL
    )

    set(_nerd_domain_names ${_nerd_domain_names} ${domain_name} PARENT_SCOPE)
    string(APPEND _nerd_domains_yaml
        "  - name: ${domain_name}\n"
        "    board: ${board_target}\n"
        "    build_dir: ${domain_binary_dir}\n"
        "    config: ${domain_binary_dir}/.config\n"
        "    autoconf: ${domain_binary_dir}/kconfig/include/generated/autoconf.h\n"
        "    devicetree: ${domain_binary_dir}/kconfig/include/generated/devicetree_generated.h\n"
        "    hw_metadata: ${domain_binary_dir}/kconfig/include/generated/nerd_hw_metadata.h\n"
        "    linker_memory: ${domain_binary_dir}/kconfig/include/generated/nerd_memory_regions.ld\n"
        "    layers: ${domain_binary_dir}/build_layers.txt\n"
        "    elf: ${domain_binary_dir}/legacy-bsp/nerd.elf\n"
        "    uf2: ${domain_binary_dir}/legacy-bsp/nerd.uf2\n"
    )
    set(_nerd_domains_yaml "${_nerd_domains_yaml}" PARENT_SCOPE)
endfunction()

if(SB_CONFIG_PICO2_M33_DOMAIN)
    nerd_add_domain(pico2_m33 "pico2/rp2350a/m33")
endif()

if(SB_CONFIG_PICO2_HAZARD3_DOMAIN)
    nerd_add_domain(pico2_hazard3 "pico2/rp2350a/hazard3")
endif()

if(NOT _nerd_domain_names)
    message(FATAL_ERROR "No sysbuild domains enabled")
endif()

add_custom_target(domains DEPENDS ${_nerd_domain_names})
add_custom_target(domains_configure)
foreach(domain_name ${_nerd_domain_names})
    add_dependencies(domains_configure ${domain_name}-configure)
endforeach()

file(WRITE "${CMAKE_BINARY_DIR}/domains.yaml" "${_nerd_domains_yaml}")
file(WRITE "${CMAKE_BINARY_DIR}/domains.txt" "")
foreach(domain_name ${_nerd_domain_names})
    file(APPEND "${CMAKE_BINARY_DIR}/domains.txt" "${domain_name}\n")
endforeach()

message(STATUS "NerdRTOS sysbuild domains: ${_nerd_domain_names}")
message(STATUS "NerdRTOS sysbuild metadata: ${CMAKE_BINARY_DIR}/domains.yaml")