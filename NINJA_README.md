# ARM M33
cmake -S . -B build-pico2-arm -GNinja -DBUILD_CONFIG=pico2_arm -DPICO_BOARD=pico2
ninja -C build-pico2-arm

# RISC-V RV32
cmake -S . -B build-pico2-riscv -GNinja -DBUILD_CONFIG=pico2_riscv -DPICO_PLATFORM=rp2350-riscv
ninja -C build-pico2-riscv

# menuconfig
ninja -C build-pico2-arm menuconfig
ninja -C build-pico2-riscv menuconfig
