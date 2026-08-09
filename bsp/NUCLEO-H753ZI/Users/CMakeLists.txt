set(DEVICE_PATH ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32H7xx)
set(START_UP         ${DEVICE_PATH}/Source/Templates/gcc/startup_stm32h753xx.s)
set(SYS_CLOCK_SOURCE ${DEVICE_PATH}/Source/Templates/system_stm32h7xx.c)

set(ARCH_PATH ${CMAKE_SOURCE_DIR}/../../arch/arm/m7)
file(GLOB_RECURSE ARCH ${ARCH_PATH}/*.c ${ARCH_PATH}/*.S)

set(NERD_KERNEL_SOURCE_PATH ${CMAKE_SOURCE_DIR}/../../kernel)
set(NERD_KERNEL_HEAD_PATH   ${CMAKE_SOURCE_DIR}/../../include)
file(GLOB_RECURSE NERD_KERNEL_SOURCE ${NERD_KERNEL_SOURCE_PATH}/*.c)

set(COMPONENTS_PATH ${CMAKE_SOURCE_DIR}/../../components)
file(GLOB_RECURSE COMPONENTS_SOURCE ${COMPONENTS_PATH}/*.c)

list(FILTER COMPONENTS_SOURCE EXCLUDE REGEX "/lwip/")

set(LWIP_DIR ${COMPONENTS_PATH}/lwip)
include(${LWIP_DIR}/lwip.cmake)

set(LIB_PATH ${CMAKE_SOURCE_DIR}/../../lib)
file(GLOB_RECURSE LIB_SOURCE ${LIB_PATH}/*.c)

set(BSP_PATH ${CMAKE_SOURCE_DIR}/Drivers/BSP)
file(GLOB BSP_SUBDIRS LIST_DIRECTORIES true ${BSP_PATH}/*)
file(GLOB_RECURSE BSP_SOURCE ${BSP_PATH}/*.c)

file(GLOB USER_SOURCE *.c)

add_executable(Firmware
    ${USER_SOURCE}
    ${START_UP}
    ${SYS_CLOCK_SOURCE}
    ${BSP_SOURCE}
    ${ARCH}
    ${NERD_KERNEL_SOURCE}
    ${COMPONENTS_SOURCE}
    ${NERDRTOS_LWIP_SOURCES}
    ${LIB_SOURCE}
)

target_link_libraries(Firmware
    Drivers
)

target_include_directories(Firmware PRIVATE
    ${COMPONENTS_PATH}/ntest/include
    ${COMPONENTS_PATH}/shell/include
    ${NERDRTOS_LWIP_INCLUDE_DIRS}
    ${NERD_KERNEL_HEAD_PATH}
    ${NERD_KERNEL_HEAD_PATH}/lib
    # ST stm32-lan8742 component, version v1.0.4.
    ${BSP_PATH}/Components/lan8742
    ${CMAKE_SOURCE_DIR}/Users
)
foreach(dir IN LISTS BSP_SUBDIRS)
    if(IS_DIRECTORY ${dir})
        target_include_directories(Firmware PRIVATE ${dir})
    endif()
endforeach()

set(LINKER_FLAGS
    -T${CMAKE_SOURCE_DIR}/stm32h753zi.ld
    -Wl,-Map=${PROJECT_NAME}.map
    -Wl,--gc-sections
    -static
    -nostartfiles
    --specs=nosys.specs
)

target_compile_options(Firmware PRIVATE
    ${MCU_FLAGS}
    ${WARNING_FLAGS}
    $<$<CONFIG:Debug>:${DEBUG_OPT}>
    $<$<CONFIG:Release>:${RELEASE_OPT}>
)

target_link_options(Firmware PRIVATE
    ${MCU_FLAGS}
    ${LINKER_FLAGS}
)

target_compile_definitions(Firmware PRIVATE
    USE_HAL_DRIVER
    STM32H753xx
)

add_custom_command(TARGET Firmware POST_BUILD
    COMMAND arm-none-eabi-objcopy -O ihex   $<TARGET_FILE:Firmware> $<TARGET_FILE_DIR:Firmware>/Firmware.hex
    COMMAND arm-none-eabi-objcopy -O binary $<TARGET_FILE:Firmware> $<TARGET_FILE_DIR:Firmware>/Firmware.bin
    COMMENT "Generating Firmware.hex and Firmware.bin"
)
