set(MYRTOS_SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/qemu_cortex_a9_soc.ld PARENT_SCOPE)

add_library(soc STATIC soc_init.c)
target_include_directories(soc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(soc PUBLIC arch myrtos_kernel)