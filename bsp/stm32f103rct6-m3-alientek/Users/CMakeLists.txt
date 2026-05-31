file(GLOB START_UP
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/gcc/startup_stm32f103xe.s
)

set(SYS_CLOCK_SOURCE
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/system_stm32f1xx.c
)

# Set path and found file
# Cortex-M3 arch
set(ARCH_PATH ${CMAKE_SOURCE_DIR}/../../arch/arm/m3)
file(GLOB_RECURSE ARCH
    ${ARCH_PATH}/*.c
    ${ARCH_PATH}/*.S 
)
# Nerd-RTOS kernel
set(NERD_KERNEL_SOURCE_PATH ${CMAKE_SOURCE_DIR}/../../kernel)
set(NERD_KERNEL_HEAD_PATH ${CMAKE_SOURCE_DIR}/../../include)
file(GLOB_RECURSE NERD_KERNEL_SOURC
    ${NERD_KERNEL_SOURCE_PATH}/*.c 
)
# Components
set(COMPONENTS_PATH ${CMAKE_SOURCE_DIR}/../../components)
file(GLOB_RECURSE COMPONENTS_SOURCE
    ${COMPONENTS_PATH}/*.c 
)
# Lib
set(LIB_PATH ${CMAKE_SOURCE_DIR}/../../lib)
file(GLOB_RECURSE LIB_SOURCE
    ${LIB_PATH}/*.c 
)
# BSP
set(BSP_PATH ${CMAKE_SOURCE_DIR}/Drivers/BSP)
file(GLOB BSP_SUBDIRS
    LIST_DIRECTORIES true
    ${BSP_PATH}/*
)
file(GLOB_RECURSE BSP_HEAD 
    ${BSP_PATH}/*.h
)
file(GLOB_RECURSE BSP_SOURCE 
    ${BSP_PATH}/*.c 
)

add_executable(Firmware
    main.c
    app.c 
    shell_port.c 
    ${START_UP}
    ${SYS_CLOCK_SOURCE}
    ${ARCH}
    ${NERD_KERNEL_SOURC}
    ${COMPONENTS_SOURCE}
    ${LIB_SOURCE}
    ${BSP_SOURCE}
)

target_link_libraries(Firmware
    Drivers
)

target_include_directories(Firmware PRIVATE
    # Components include path
    ${COMPONENTS_PATH}/ntest/include
    ${COMPONENTS_PATH}/shell/include
    # Nerd-RTOS core include path
    ${NERD_KERNEL_HEAD_PATH} 
    ${NERD_KERNEL_HEAD_PATH}/lib 
    # User include path
    ${CMAKE_SOURCE_DIR}/Users 
)
# BSP include path
foreach(dir IN LISTS BSP_SUBDIRS)
    if(IS_DIRECTORY ${dir})
        target_include_directories(Firmware PRIVATE
            ${dir}
        )
    endif()
endforeach()

set(LINKER_FLAGS
    -T${CMAKE_SOURCE_DIR}/stm32f103rct6.ld
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
    STM32F103xE
)

# Generate .hex and .bin for flashing
add_custom_command(TARGET Firmware POST_BUILD
    COMMAND arm-none-eabi-objcopy -O ihex   $<TARGET_FILE:Firmware> $<TARGET_FILE_DIR:Firmware>/Firmware.hex
    COMMAND arm-none-eabi-objcopy -O binary $<TARGET_FILE:Firmware> $<TARGET_FILE_DIR:Firmware>/Firmware.bin
    COMMENT "Generating Firmware.hex and Firmware.bin"
)
