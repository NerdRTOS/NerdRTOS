# Pico 2 board metadata needed before project() and Pico SDK import.
# Keep this file free of Kconfig symbols; Kconfig has not run yet.

if("${NERD_BOARD_HINT}" STREQUAL "pico2/rp2350a/hazard3")
    set(PICO_BOARD pico2 CACHE STRING "Pico SDK board selected by BOARD")
    set(PICO_PLATFORM rp2350-riscv CACHE STRING "Pico SDK platform selected by BOARD")
elseif("${NERD_BOARD_HINT}" STREQUAL "pico2"
       OR "${NERD_BOARD_HINT}" STREQUAL "pico2/rp2350a/m33")
    set(PICO_BOARD pico2 CACHE STRING "Pico SDK board selected by BOARD")
elseif("${NERD_BOARD_HINT}" MATCHES "^pico2/")
    message(FATAL_ERROR "Unsupported Pico 2 BOARD target: ${NERD_BOARD_HINT}")
endif()