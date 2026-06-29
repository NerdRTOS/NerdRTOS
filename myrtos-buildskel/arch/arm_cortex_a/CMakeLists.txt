set(MYRTOS_TOOLCHAIN_PROFILE arm-none-eabi-cortex-a PARENT_SCOPE)
set(MYRTOS_ARCH_STARTUP_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/startup_arm_cortex_a.c PARENT_SCOPE)
set(MYRTOS_ARCH_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker_arm_cortex_a.ld PARENT_SCOPE)

add_library(arch STATIC startup_arm_cortex_a.c)
target_include_directories(arch PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(arch PUBLIC myrtos_kernel)