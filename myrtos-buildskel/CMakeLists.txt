cmake_minimum_required(VERSION 3.20)
project(myrtos C)

# 1) 跑 Kconfig + DTS，得到 CONFIG_xxx CMake 变量和生成头
include(cmake/kconfig.cmake)

# 2) 内核层：任何 arch/soc/board/app 都要能 #include 生成头
add_library(myrtos_kernel INTERFACE)
target_include_directories(myrtos_kernel INTERFACE
    ${CMAKE_BINARY_DIR}/include/generated
)

# 3) 根据 Kconfig 推导出的 ARCH/BOARD 选择 arch/soc/board 目录。
if(CONFIG_ARCH_ARM_CORTEX_M)
    set(ARCH_DIR arch/arm_cortex_m)
elseif(CONFIG_ARCH_ARM_CORTEX_A)
    set(ARCH_DIR arch/arm_cortex_a)
elseif(CONFIG_ARCH_RISCV)
    set(ARCH_DIR arch/riscv)
else()
    message(FATAL_ERROR "没有任何 ARCH_xxx 被选中")
endif()

if(CONFIG_BOARD_PICO2)
    set(SOC_DIR   soc/raspberrypi/rp2350)
    set(BOARD_DIR boards/raspberrypi/pico2)
elseif(CONFIG_BOARD_QEMU_A9)
    set(SOC_DIR   soc/qemu/cortex_a9)
    set(BOARD_DIR boards/qemu/qemu_a9)
elseif(CONFIG_BOARD_NUCLEO_F429ZI)
    set(SOC_DIR   soc/st/stm32f4)
    set(BOARD_DIR boards/st/nucleo_f429zi)
else()
    message(FATAL_ERROR
        "没有任何 BOARD_xxx 被选中，先跑一次："
        " cmake -S . -B <builddir> -DBOARD=pico2/rp2350a/m33")
endif()

message(STATUS "BOARD=${CONFIG_BOARD}  ARCH=${CONFIG_ARCH}  ARCH_DIR=${ARCH_DIR}  SOC_DIR=${SOC_DIR}  BOARD_DIR=${BOARD_DIR}")

add_subdirectory(${ARCH_DIR}  arch_build)
add_subdirectory(${SOC_DIR}   soc_build)
add_subdirectory(${BOARD_DIR} board_build)
add_subdirectory(app)

# 4) 这一阶段不强制 host linker 使用裸机链接脚本，只生成构建分层摘要。
#    真正迁移到 NerdRTOS 主工程时，这些变量会接到交叉工具链和 target_link_options。
set(MYRTOS_BUILD_LAYERS_TXT ${CMAKE_BINARY_DIR}/build_layers.txt)
file(WRITE ${MYRTOS_BUILD_LAYERS_TXT}
"BOARD=${CONFIG_BOARD}\n"
"ARCH=${CONFIG_ARCH}\n"
"ARCH_DIR=${ARCH_DIR}\n"
"SOC_DIR=${SOC_DIR}\n"
"BOARD_DIR=${BOARD_DIR}\n"
"TOOLCHAIN_PROFILE=${MYRTOS_TOOLCHAIN_PROFILE}\n"
"ARCH_STARTUP_SOURCE=${MYRTOS_ARCH_STARTUP_SOURCE}\n"
"ARCH_LINKER_SCRIPT=${MYRTOS_ARCH_LINKER_SCRIPT}\n"
"SOC_LINKER_SCRIPT=${MYRTOS_SOC_LINKER_SCRIPT}\n"
"BOARD_LINKER_SCRIPT=${MYRTOS_BOARD_LINKER_SCRIPT}\n"
)
message(STATUS "Build layer summary: ${MYRTOS_BUILD_LAYERS_TXT}")