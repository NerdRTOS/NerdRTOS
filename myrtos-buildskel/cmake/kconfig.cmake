# cmake/kconfig.cmake
#
# 这个文件做的事情，对应 Zephyr/ESP-IDF 在 "configure" 阶段做的事情：
#   1. 确定 BOARD（必须由用户通过 -DBOARD=xxx 指定）
#   2. 支持 Zephyr 式 board target/qualifier，例如 pico2/rp2350a/m33
#   3. 第一次 configure 时合并 board defconfig + app/prj.conf + app/boards/<board>.conf
#   4. 调用 genconfig.py 跑一遍完整的 Kconfig 树，merge + 校验依赖 + 落地
#      .config 和 autoconf.h
#   5. 把 autoconf.h 里的 CONFIG_xxx 解析成 CMake 变量，
#      这样上层 CMakeLists.txt 才能用 if(CONFIG_BOARD_PICO2) 做分支
#   6. 注册一个 `menuconfig` target，方便交互式改配置

if(NOT DEFINED BOARD)
    message(FATAL_ERROR
        "必须指定板子，例如：cmake -S . -B build -DBOARD=pico2/rp2350a/m33\n"
        "可选值：pico2/rp2350a/m33 | pico2/rp2350a/hazard3 | qemu_a9 | nucleo_f429zi")
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(KCONFIG_ROOT     ${CMAKE_SOURCE_DIR}/Kconfig)
set(DOTCONFIG        ${CMAKE_BINARY_DIR}/.config)
set(AUTOCONF_H       ${CMAKE_BINARY_DIR}/include/generated/autoconf.h)
set(DEVICETREE_H    ${CMAKE_BINARY_DIR}/include/generated/devicetree_generated.h)
set(APP_DIR          ${CMAKE_SOURCE_DIR}/app)
set(KCONFIG_SCRIPTS  ${CMAKE_SOURCE_DIR}/scripts/kconfig)
set(DTS_SCRIPTS      ${CMAKE_SOURCE_DIR}/scripts/dts)
set(KCONFIG_CHECKSUM ${CMAKE_BINARY_DIR}/.config.fragment.checksum)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/include/generated)

set(BOARD_TARGET ${BOARD})
if(BOARD_TARGET STREQUAL "pico2")
    set(BOARD_TARGET "pico2/rp2350a/m33")
    message(STATUS "BOARD=pico2 is an alias for BOARD=${BOARD_TARGET}")
endif()

set(_valid_boards
    pico2/rp2350a/m33
    pico2/rp2350a/hazard3
    qemu_a9
    nucleo_f429zi
)

if(NOT BOARD_TARGET IN_LIST _valid_boards)
    message(FATAL_ERROR
        "未知 BOARD='${BOARD}'.\n"
        "可选值：pico2/rp2350a/m33 | pico2/rp2350a/hazard3 | qemu_a9 | nucleo_f429zi")
endif()

string(REPLACE "/" "_" BOARD_NORMALIZED ${BOARD_TARGET})
string(REPLACE "-" "_" BOARD_NORMALIZED ${BOARD_NORMALIZED})
string(TOUPPER ${BOARD_NORMALIZED} BOARD_UPPER)

if(BOARD_TARGET STREQUAL "pico2/rp2350a/m33")
    set(BOARD_DEFCONFIG ${CMAKE_SOURCE_DIR}/boards/raspberrypi/pico2/pico2_rp2350a_m33_defconfig)
    set(DEVICETREE_SOURCE ${CMAKE_SOURCE_DIR}/boards/raspberrypi/pico2/pico2_rp2350a_m33.dts)
elseif(BOARD_TARGET STREQUAL "pico2/rp2350a/hazard3")
    set(BOARD_DEFCONFIG ${CMAKE_SOURCE_DIR}/boards/raspberrypi/pico2/pico2_rp2350a_hazard3_defconfig)
    set(DEVICETREE_SOURCE ${CMAKE_SOURCE_DIR}/boards/raspberrypi/pico2/pico2_rp2350a_hazard3.dts)
elseif(BOARD_TARGET STREQUAL "qemu_a9")
    set(BOARD_DEFCONFIG ${CMAKE_SOURCE_DIR}/boards/qemu/qemu_a9/qemu_a9_defconfig)
    set(DEVICETREE_SOURCE ${CMAKE_SOURCE_DIR}/boards/qemu/qemu_a9/qemu_a9.dts)
elseif(BOARD_TARGET STREQUAL "nucleo_f429zi")
    set(BOARD_DEFCONFIG ${CMAKE_SOURCE_DIR}/boards/st/nucleo_f429zi/nucleo_f429zi_defconfig)
    set(DEVICETREE_SOURCE ${CMAKE_SOURCE_DIR}/boards/st/nucleo_f429zi/nucleo_f429zi.dts)
endif()

if(NOT EXISTS ${BOARD_DEFCONFIG})
    message(FATAL_ERROR "Board defconfig not found: ${BOARD_DEFCONFIG}")
endif()
if(NOT EXISTS ${DEVICETREE_SOURCE})
    message(FATAL_ERROR "Devicetree source not found: ${DEVICETREE_SOURCE}")
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${DEVICETREE_SOURCE})

execute_process(
    COMMAND ${Python3_EXECUTABLE} ${DTS_SCRIPTS}/gen_devicetree.py
            ${DEVICETREE_SOURCE} ${DEVICETREE_H}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE DTS_RESULT
    OUTPUT_VARIABLE DTS_OUTPUT
    ERROR_VARIABLE  DTS_ERROR
)

if(NOT DTS_RESULT EQUAL 0)
    message(FATAL_ERROR "Devicetree 生成失败:\n${DTS_OUTPUT}\n${DTS_ERROR}")
endif()
message(STATUS "${DTS_OUTPUT}")

set(APP_CONF ${APP_DIR}/prj.conf)
set(APP_BOARD_CONF ${APP_DIR}/boards/${BOARD_NORMALIZED}.conf)

set(KCONFIG_FRAGMENT_FILES ${BOARD_DEFCONFIG})
if(EXISTS ${APP_CONF})
    list(APPEND KCONFIG_FRAGMENT_FILES ${APP_CONF})
endif()
if(EXISTS ${APP_BOARD_CONF})
    list(APPEND KCONFIG_FRAGMENT_FILES ${APP_BOARD_CONF})
endif()

set(_fragment_checksum "")
foreach(_fragment ${KCONFIG_FRAGMENT_FILES})
    file(MD5 ${_fragment} _checksum)
    string(APPEND _fragment_checksum "${_checksum}")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${_fragment})
endforeach()

set(CREATE_NEW_DOTCONFIG TRUE)
if(EXISTS ${DOTCONFIG} AND EXISTS ${KCONFIG_CHECKSUM})
    file(READ ${KCONFIG_CHECKSUM} _fragment_checksum_prev)
    if(_fragment_checksum STREQUAL _fragment_checksum_prev)
        set(CREATE_NEW_DOTCONFIG FALSE)
    endif()
endif()

if(CREATE_NEW_DOTCONFIG)
    set(KCONFIG_INPUT_CONFIGS ${KCONFIG_FRAGMENT_FILES})
else()
    set(KCONFIG_INPUT_CONFIGS ${DOTCONFIG})
endif()

message(STATUS "Kconfig fragments: ${KCONFIG_FRAGMENT_FILES}")

# osource "$(APP_DIR)/Kconfig" 需要这个环境变量
set(ENV{APP_DIR} ${APP_DIR})
# kconfiglib 里所有 source/rsource 路径都是相对 $srctree 解析的，
# 不设置的话它会用当前工作目录，容易因为 WORKING_DIRECTORY 不对而找不到文件
set(ENV{srctree} ${CMAKE_SOURCE_DIR})

execute_process(
    COMMAND ${Python3_EXECUTABLE} ${KCONFIG_SCRIPTS}/genconfig.py
            ${KCONFIG_ROOT} ${DOTCONFIG} ${AUTOCONF_H} ${KCONFIG_INPUT_CONFIGS}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE KCONFIG_RESULT
    OUTPUT_VARIABLE KCONFIG_OUTPUT
    ERROR_VARIABLE  KCONFIG_ERROR
)

if(NOT KCONFIG_RESULT EQUAL 0)
    message(FATAL_ERROR "Kconfig 生成失败:\n${KCONFIG_OUTPUT}\n${KCONFIG_ERROR}")
endif()
message(STATUS "${KCONFIG_OUTPUT}")

if(CREATE_NEW_DOTCONFIG)
    file(WRITE ${KCONFIG_CHECKSUM} ${_fragment_checksum})
endif()

# ---- 把 autoconf.h 里的 #define CONFIG_XXX VALUE 解析成 CMake 变量 ----
file(STRINGS ${AUTOCONF_H} _autoconf_lines REGEX "^#define CONFIG_")
foreach(_line ${_autoconf_lines})
    string(REGEX MATCH "^#define ([A-Za-z0-9_]+) (.*)$" _ ${_line})
    if(CMAKE_MATCH_1)
        set(${CMAKE_MATCH_1} "${CMAKE_MATCH_2}")
    endif()
endforeach()

set(CONFIG_BOARD_TARGET ${CONFIG_BOARD})
string(REGEX REPLACE "^\"(.*)\"$" "\\1" CONFIG_BOARD_TARGET "${CONFIG_BOARD_TARGET}")
if(NOT CONFIG_BOARD_TARGET STREQUAL BOARD_TARGET)
    message(FATAL_ERROR
        "当前 build 目录里的 .config 选择的是 BOARD=${CONFIG_BOARD_TARGET}，"
        "但本次 CMake 参数是 BOARD=${BOARD_TARGET}。\n"
        "请换一个 build 目录，或者删除 ${DOTCONFIG} 后重新 configure。")
endif()

# ---- 交互式配置入口：cmake --build . --target menuconfig ----
add_custom_target(menuconfig
    COMMAND ${CMAKE_COMMAND} -E env "srctree=${CMAKE_SOURCE_DIR}" "APP_DIR=${APP_DIR}"
            ${Python3_EXECUTABLE} ${KCONFIG_SCRIPTS}/menuconfig.py
            ${KCONFIG_ROOT} ${DOTCONFIG}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    USES_TERMINAL
    COMMENT "退出并保存后，需要重新执行一次 cmake 才会重新生成 autoconf.h"
)