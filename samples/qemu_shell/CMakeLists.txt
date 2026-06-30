# Zephyr-style QEMU shell sample application.
set(NERD_SAMPLE_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/app.c
    CACHE INTERNAL "Selected sample application sources"
)

set(NERD_SAMPLE_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/include
    CACHE INTERNAL "Selected sample application include directories"
)
