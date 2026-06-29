set(MYRTOS_TOOLCHAIN_PROFILE riscv-none-elf-rv32 PARENT_SCOPE)
set(MYRTOS_ARCH_STARTUP_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/startup_riscv.c PARENT_SCOPE)
set(MYRTOS_ARCH_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker_riscv.ld PARENT_SCOPE)

add_library(arch STATIC startup_riscv.c)
target_include_directories(arch PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(arch PUBLIC myrtos_kernel)