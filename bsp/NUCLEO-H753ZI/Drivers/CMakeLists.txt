file(GLOB HAL_SOURCES
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/*.c
)
# Templates define HAL_MspInit/HAL_InitTick — would clash with user code
list(FILTER HAL_SOURCES EXCLUDE REGEX "_template\\.c$")

add_library(Drivers STATIC
    ${HAL_SOURCES}
)

target_include_directories(Drivers PUBLIC
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Include
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32H7xx/Include
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7xx_HAL_Driver/Inc
    ${CMAKE_SOURCE_DIR}/Users
)

target_compile_options(Drivers PRIVATE
    ${MCU_FLAGS}
    ${WARNING_FLAGS}
    $<$<CONFIG:Debug>:${DEBUG_OPT}>
    $<$<CONFIG:Release>:${RELEASE_OPT}>
)

target_compile_definitions(Drivers PUBLIC
    USE_HAL_DRIVER
    STM32H753xx
)
