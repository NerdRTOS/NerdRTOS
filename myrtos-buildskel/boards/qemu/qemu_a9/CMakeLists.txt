set(MYRTOS_BOARD_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/qemu_a9_board.ld PARENT_SCOPE)

add_library(board STATIC board_init.c)
target_include_directories(board PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(board PUBLIC soc myrtos_kernel)