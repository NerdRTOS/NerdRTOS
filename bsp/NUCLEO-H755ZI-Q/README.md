# NUCLEO-H755ZI-Q — Cortex-M4

This BSP runs NerdRTOS on the STM32H755 Cortex-M4. The Cortex-M7 runs a
minimal startup image, wakes the M4, and then waits in normal sleep. Both
images are needed. This is not an M4-only hardware boot configuration.

The startup sequence follows ST's `NUCLEO-H745ZI-Q/Templates/BootCM4_CM7`
example. ST's H755ZI-Q project readme declares the H745ZI-Q examples
compatible with this board. HSEM 0 is used only for startup synchronization;
this BSP does not provide an AMP transport or shared-memory protocol.

## Configuration

| Resource | Configuration |
| --- | --- |
| Clock | Internal HSI, 64 MHz; no PLL or external oscillator required |
| Supply | Direct SMPS, stock NUCLEO-H755ZI-Q supply configuration |
| M7 image | `Boot_CM7.elf`, Flash `0x08000000`, 1 MiB bank |
| M7 RAM | DTCM `0x20000000`, 128 KiB |
| M4 image | `Firmware_CM4.elf`, Flash `0x08100000`, 1 MiB bank |
| M4 RAM | D2 SRAM alias `0x10000000`, 288 KiB |
| M4 tick | SysTick, 1 kHz |
| Console | USART3, PD8/PD9, ST-LINK VCP, 115200-8N1 |
| M4 FPU | FPv4-SP-D16, hard-float ABI |

The supply configuration must match the physical board. This BSP targets
NUCLEO-H755ZI-Q, not NUCLEO-H753ZI or a custom H755 board.

Boot option bytes must enable both Cortex-M7 and Cortex-M4 boot, with
their boot addresses set to `0x08000000` and `0x08100000`, respectively.
Use the normal user-Flash boot mode. Check these settings with
STM32CubeProgrammer if the board's option bytes have been changed.
The commands below do not modify option bytes.

## Dependencies

Install `arm-none-eabi-gcc`, CMake, and an OpenOCD build with ST-LINK SWD
and STM32H7 dual-core support. Vendor sources are not committed.
From the repository root:

```sh
git clone --depth 1 --branch v1.13.0 \
  https://github.com/STMicroelectronics/STM32CubeH7.git /tmp/STM32CubeH7
git -C /tmp/STM32CubeH7 submodule update --init --depth 1 \
  Drivers/CMSIS/Device/ST/STM32H7xx Drivers/STM32H7xx_HAL_Driver
cp -r /tmp/STM32CubeH7/Drivers/CMSIS bsp/NUCLEO-H755ZI-Q/Drivers/
cp -r /tmp/STM32CubeH7/Drivers/STM32H7xx_HAL_Driver \
  bsp/NUCLEO-H755ZI-Q/Drivers/
```

Alternatively, pass `-DSTM32_DRIVERS_DIR=/absolute/path/to/Drivers`
to use an existing STM32CubeH7 v1.13.0 driver tree. Only CMSIS and the
LL HSEM header are compiled/used; no HAL tick competes with the RTOS tick.
The vendor dual-core system source is used for both images, with separate
`CORE_CM4` and `CORE_CM7` definitions. CMake adapts the generic startup
assembly's CPU directive for M4 in the build directory.

## Build

```sh
cmake -S bsp/NUCLEO-H755ZI-Q -B build/NUCLEO-H755ZI-Q \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build/NUCLEO-H755ZI-Q -j
```

Both ELF files, corresponding HEX/BIN files, and linker maps are generated
in `build/NUCLEO-H755ZI-Q`. Release builds should use a separate build
directory and `-DCMAKE_BUILD_TYPE=Release`.

## Flash

Connect the H755 board's ST-LINK USB port and close other OpenOCD/GDB
sessions using that probe. From the repository root:

```sh
openocd -f bsp/NUCLEO-H755ZI-Q/openocd.cfg \
  -c "program build/NUCLEO-H755ZI-Q/Boot_CM7.elf verify" \
  -c "program build/NUCLEO-H755ZI-Q/Firmware_CM4.elf verify reset exit"
```

This writes both images and resets the device. Do not substitute the H753
firmware. After flashing, perform a power cycle if the board was previously
running a different supply configuration.

## Console and debug

Open the ST-LINK VCP at 115200-8N1, then reset the board. Expected banner:

```text
Hello Nerd RTOS! H755 Cortex-M4, HSI 64 MHz
```

Use `help` to list shell commands and `ps` to inspect threads.
The in-tree `test_*` commands are available through the shell.

To debug, start OpenOCD with the same configuration, without the `program`
commands. M7 uses GDB port 3333; M4 uses 3334. Load `Firmware_CM4.elf` in
GDB and connect to port 3334. If both cores are halted by a reset, resume
M7 as well as M4 so the startup handshake can complete. Test cold boot
with both images installed rather than starting M4 alone from its ELF.

If M7 remains in `boot_fail`, inspect `boot_error` in `Boot_CM7.elf`:

| Value | Meaning |
| --- | --- |
| 1 | M4 did not enter D2 STOP before timeout |
| 2 | The clock is not the expected 64 MHz |
| 3 | Startup HSEM acquisition failed |
| 4 | M4 did not wake before timeout |

## Validation status

Debug and Release builds and ELF layout checks are performed during BSP
development. H755 hardware boot, serial I/O, scheduling, and floating-point
context switching still require on-board verification. Compilation is not
evidence of successful hardware bring-up.

## References

- [ST H755ZI-Q compatibility note](https://github.com/STMicroelectronics/STM32CubeH7/blob/v1.13.0/Projects/NUCLEO-H755ZI-Q/readme.txt)
- [ST dual-core boot template](https://github.com/STMicroelectronics/STM32CubeH7/tree/v1.13.0/Projects/NUCLEO-H745ZI-Q/Templates/BootCM4_CM7)
- [NUCLEO board manual UM2408](https://www.st.com/resource/en/user_manual/um2408-stm32h7-nucleo144-boards-mb1363-stmicroelectronics.pdf)
