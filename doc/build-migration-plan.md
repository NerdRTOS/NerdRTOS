# NerdRTOS Zephyr-style Build Migration Plan

This document records the migration plan from the current BSP-driven build to a Zephyr-style build architecture.
The reference implementation is kept in `myrtos-buildskel/` and should remain available until the main tree migration is complete.

## Scope

The migration target is the real NerdRTOS tree, not the experimental skeleton.

Current reference implementation:

```text
myrtos-buildskel/
```

Current production tree:

```text
arch/
bsp/
cmake/
components/
configs/
include/
kernel/
lib/
mm/
scripts/
```

The migration must be incremental. Existing working BSP flows should not be deleted until the replacement path can configure, build, and run or flash.

## Current Build Model

The current top-level build is BSP-first:

```text
BUILD_CONFIG=pico2
  -> configs/pico2_defconfig
  -> CONFIG_BSP_RASPI_PICO2
  -> CONFIG_ARCH_ARM_M33 or CONFIG_ARCH_RISCV_RV32
  -> add_subdirectory(bsp/raspi-pico2)
```

Current limitations:

- `bsp/raspi-pico2` is the only BSP integrated through the top-level `CMakeLists.txt`.
- `bsp/vexpress-a9-qemu` and `bsp/stm32f103rct6-m3-alientek` exist, but they are standalone CMake trees instead of domains in one unified build model.
- Architecture is still user-selected in Kconfig instead of being derived from the board target.
- Hardware facts are split between Kconfig, C headers, linker scripts, and BSP code.
- Multi-image or multi-domain builds are not represented at the top level.

## Target Build Model

The target model is board-target-first:

```text
BOARD=<board-target>
  -> board defconfig
  -> board selects SoC
  -> SoC selects arch
  -> DTS describes hardware facts
  -> CMake selects arch/soc/board layers and a sample application
```

Initial board targets:

```text
pico2/rp2350a/m33
pico2/rp2350a/hazard3
qemu_a9/cortex_a9
stm32f103rct6/m3
```

The target CMake dependency chain is:

```text
sample app -> board -> soc -> arch -> nerd_kernel
```

The target sysbuild shape is:

```text
sysbuild
  -> domains/pico2_m33
  -> domains/pico2_hazard3
  -> domains/qemu_a9
  -> domains/stm32f103rct6
```

## Reference Features Already Proven in myrtos-buildskel

The skeleton has already validated these concepts:

- Board target qualifiers, including `pico2/rp2350a/m33` and `pico2/rp2350a/hazard3`.
- Kconfig select chain: board -> SoC -> arch.
- Config fragment merge order: board defconfig -> `samples/<sample>/prj.conf` -> `samples/<sample>/boards/<board>.conf`.
- Minimal DTS -> `devicetree_generated.h` generation.
- CMake layer chain: `sample app -> board -> soc -> arch -> myrtos_kernel`.
- Build layer metadata in `build_layers.txt`.
- Simplified sysbuild/domain using CMake `ExternalProject`.

## Migration Rules

1. Keep `myrtos-buildskel/` as the reference until the main tree has equivalent behavior.
2. Do not delete the old `bsp/` entry points until the new board/soc/arch path builds the same target.
3. Prefer adding new wrappers first, then moving source files only after the build shape is stable.
4. Keep each migration step independently verifiable.
5. When moving code, preserve behavior before refactoring style.
6. Separate hardware facts from software switches:
   - DTS: memory, flash, UART, timer, IRQ, chosen devices, boot-time hardware configuration.
   - Kconfig: software features, kernel options, component inclusion, debug/test options.

## Step 1: Add New Hardware Model Directories

Status: completed as wrapper-only scaffolding. Existing top-level CMakeLists.txt and Kconfig are intentionally unchanged in this step.

Add new directories without changing build behavior:

```text
boards/
  raspberrypi/pico2/
  qemu/qemu_a9/
  st/stm32f103rct6_m3_alientek/

soc/
  raspberrypi/rp2350/
  qemu/cortex_a9/
  st/stm32f1/

arch/
  arm/cortex_m/
  arm/cortex_a/
  riscv/rv32/
```

Notes:

- The repository already has `arch/arm/...` and `arch/riscv/rv32/...`; do not move them immediately.
- New directories can initially contain Kconfig/CMake wrappers that point to old locations.

Metadata aliases exported by `nerd_hw_metadata.h`:

```text
NERD_BOARD_MODEL / NERD_BOARD_COMPATIBLE
NERD_BOARD_CONSOLE_LABEL / NERD_BOARD_CONSOLE_BASE / NERD_BOARD_CONSOLE_IRQ / NERD_BOARD_CONSOLE_BAUD
NERD_SOC_SRAM_BASE / NERD_SOC_SRAM_SIZE
NERD_SOC_FLASH_BASE / NERD_SOC_FLASH_SIZE
NERD_SOC_TIMER_LABEL / NERD_SOC_TIMER_BASE / NERD_SOC_TIMER_IRQ / NERD_SOC_TIMER_CLOCK_FREQUENCY
```
Validation:

- Existing `BUILD_CONFIG=pico2` flow still configures as before.
- No existing source files are moved in this step.

## Step 2: Convert Kconfig Platform Model

Status: completed as an opt-in compatibility layer. CONFIG_BOARD_TARGET_MODEL enables the new board-target choice while existing legacy BSP defconfigs remain unchanged by default.

Replace the current BSP-first user model:

```text
CONFIG_BSP_RASPI_PICO2
CONFIG_BSP_VEXPRESS_A9_QEMU
CONFIG_BSP_STM32F103RCT6
CONFIG_ARCH_ARM_M33
CONFIG_ARCH_RISCV_RV32
```

with board-target symbols:

```text
CONFIG_BOARD_PICO2_RP2350A_M33
CONFIG_BOARD_PICO2_RP2350A_HAZARD3
CONFIG_BOARD_QEMU_A9
CONFIG_BOARD_STM32F103RCT6_M3_ALIENTEK
```

Selection chain:

```text
BOARD_PICO2_RP2350A_M33
  -> BOARD_PICO2
  -> SOC_RP2350A_M33
  -> ARCH_ARM_M33

BOARD_PICO2_RP2350A_HAZARD3
  -> BOARD_PICO2
  -> SOC_RP2350A_HAZARD3
  -> ARCH_RISCV_RV32

BOARD_QEMU_A9
  -> SOC_QEMU_CORTEX_A9
  -> ARCH_ARM_A9

BOARD_STM32F103RCT6_M3_ALIENTEK
  -> SOC_STM32F103
  -> ARCH_ARM_M3
```

Metadata aliases exported by `nerd_hw_metadata.h`:

```text
NERD_BOARD_MODEL / NERD_BOARD_COMPATIBLE
NERD_BOARD_CONSOLE_LABEL / NERD_BOARD_CONSOLE_BASE / NERD_BOARD_CONSOLE_IRQ / NERD_BOARD_CONSOLE_BAUD
NERD_SOC_SRAM_BASE / NERD_SOC_SRAM_SIZE
NERD_SOC_FLASH_BASE / NERD_SOC_FLASH_SIZE
NERD_SOC_TIMER_LABEL / NERD_SOC_TIMER_BASE / NERD_SOC_TIMER_IRQ / NERD_SOC_TIMER_CLOCK_FREQUENCY
```
Validation:

- `menuconfig` shows board target as the user-facing choice.
- Architecture is no longer selected manually by users.
- Generated `.config` contains exactly one board target and one derived arch.

## Step 3: Convert Config Fragment Layout

Status: completed as an additive layout, later corrected to a Zephyr-style sample layout. New board-target defconfigs and `samples/pico_shell/prj.conf` fragments validate through the existing Kconfig script; legacy `configs/pico2*.defconfig` files are still retained.

Move from:

```text
configs/pico2_defconfig
configs/pico2_arm_defconfig
configs/pico2_riscv_defconfig
```

to:

```text
boards/raspberrypi/pico2/pico2_rp2350a_m33_defconfig
boards/raspberrypi/pico2/pico2_rp2350a_hazard3_defconfig
boards/qemu/qemu_a9/qemu_a9_defconfig
boards/st/stm32f103rct6_m3_alientek/stm32f103rct6_m3_alientek_defconfig
app/prj.conf
app/boards/*.conf
```

Merge order:

```text
board defconfig
+ app/prj.conf
+ app/boards/<normalized-board-target>.conf
= build/.config
```

Metadata aliases exported by `nerd_hw_metadata.h`:

```text
NERD_BOARD_MODEL / NERD_BOARD_COMPATIBLE
NERD_BOARD_CONSOLE_LABEL / NERD_BOARD_CONSOLE_BASE / NERD_BOARD_CONSOLE_IRQ / NERD_BOARD_CONSOLE_BAUD
NERD_SOC_SRAM_BASE / NERD_SOC_SRAM_SIZE
NERD_SOC_FLASH_BASE / NERD_SOC_FLASH_SIZE
NERD_SOC_TIMER_LABEL / NERD_SOC_TIMER_BASE / NERD_SOC_TIMER_IRQ / NERD_SOC_TIMER_CLOCK_FREQUENCY
```
Validation:

- Reconfiguring from a clean build directory creates expected `.config`.
- Existing kernel options keep their current values.
- Board-specific app overrides work.

## Step 4: Switch Top-level CMake Shape Without Moving Code

Status: in progress. The configuration entry and the first CMake layer chain are completed. Top-level CMake now accepts a `BOARD` cache variable without forcing the legacy `BUILD_CONFIG=pico2` default, and `cmake/kconfig.cmake` can merge board-target fragments in this order:

```text
board defconfig
+ app/prj.conf
+ app/boards/<normalized-board-target>.conf
+ optional CONF_FILE/OVERLAY_CONFIG/CLI CONFIG_* overrides
```

Supported `BOARD` values at this stage:

```text
pico2
pico2/rp2350a/m33
pico2/rp2350a/hazard3
qemu_a9
qemu_a9/cortex_a9
stm32f103rct6
stm32f103rct6/m3
```

Legacy `BUILD_CONFIG=<name>` remains supported and still loads `configs/<name>_defconfig`.

The top-level CMake entry now selects wrapper directories from Kconfig when `CONFIG_BOARD_TARGET_MODEL=y`:

```cmake
add_subdirectory(${ARCH_DIR})
add_subdirectory(${SOC_DIR})
add_subdirectory(${BOARD_DIR})
add_subdirectory(${NERD_SAMPLE_DIR})
```

At this stage, new `boards/`, `soc/`, and `arch/` CMake files may still reference old source locations under `bsp/` and existing `arch/`.

Metadata aliases exported by `nerd_hw_metadata.h`:

```text
NERD_BOARD_MODEL / NERD_BOARD_COMPATIBLE
NERD_BOARD_CONSOLE_LABEL / NERD_BOARD_CONSOLE_BASE / NERD_BOARD_CONSOLE_IRQ / NERD_BOARD_CONSOLE_BAUD
NERD_SOC_SRAM_BASE / NERD_SOC_SRAM_SIZE
NERD_SOC_FLASH_BASE / NERD_SOC_FLASH_SIZE
NERD_SOC_TIMER_LABEL / NERD_SOC_TIMER_BASE / NERD_SOC_TIMER_IRQ / NERD_SOC_TIMER_CLOCK_FREQUENCY
```
Validation:

- Kconfig-only validation passed for legacy `configs/pico2_defconfig`; `CONFIG_BOARD_TARGET_MODEL` remains disabled.
- Kconfig-only validation passed for `BOARD=pico2/rp2350a/m33`; it selects `CONFIG_BOARD_PICO2_RP2350A_M33`, `CONFIG_SOC_RP2350A_M33`, and `CONFIG_ARCH_ARM_M33`.
- CMake configure passed for legacy `BUILD_CONFIG=pico2` with a temporary fake Pico SDK used only for configure-stage validation.
- CMake configure passed for `BOARD=pico2/rp2350a/m33` with a temporary fake Pico SDK used only for configure-stage validation.
- The `BOARD=pico2/rp2350a/m33` build tree contains `arch-layer`, `soc-layer`, `board-layer`, and `legacy-bsp`; cache variables confirm `NERD_ARCH_LAYER=cortex_m`, `NERD_SOC_LEGACY_BSP_DIR=bsp/raspi-pico2`, and `NERD_BOARD_LEGACY_DIR=bsp/raspi-pico2`.
- Real build/link still requires a real `PICO_SDK_PATH`; no local Pico SDK directory was found under `D:\download\GITHUB`.
- Top-level CMake no longer imports the Pico SDK for non-Pico `BOARD` values. Pico legacy and Pico BOARD paths still import the SDK before `project()`.
- `BOARD=qemu_a9/cortex_a9` and `BOARD=stm32f103rct6/m3` now reach board defconfig merge and DTS metadata generation before stopping at the expected missing `arm-none-eabi-gcc` toolchain in their legacy BSP backends.
- `BOARD=pico2/rp2350a/hazard3` configures through the new entry path. The top-level CMake includes Pico2 pre-project metadata, which sets `PICO_PLATFORM=rp2350-riscv` for this target before Pico SDK import.
- The old standalone BSP path remains available for comparison.

## Step 5: Split BSP Code by Responsibility

Status: in progress. Pico 2 board-owned sources are split into `boards/raspberrypi/pico2/`, the RP2350 timer backend is split into `soc/raspberrypi/rp2350/`, and the sample application lives in the Zephyr-style `samples/pico_shell/` tree. Legacy BSP copies are still kept for compatibility.

Completed slices:

```text
bsp/raspi-pico2/nd_led.c      -> boards/raspberrypi/pico2/nd_led.c
bsp/raspi-pico2/nd_led.h      -> boards/raspberrypi/pico2/nd_led.h
bsp/raspi-pico2/shell_port.c  -> boards/raspberrypi/pico2/shell_port.c
bsp/raspi-pico2/nd_hrtimer.c  -> soc/raspberrypi/rp2350/nd_hrtimer.c
bsp/raspi-pico2/app.c         -> samples/pico_shell/src/app.c
bsp/raspi-pico2/app.h         -> samples/pico_shell/include/app.h
```

The sample wrapper exports:

```cmake
NERD_SAMPLE_SOURCES
NERD_SAMPLE_INCLUDE_DIRS
```

The Pico board wrapper exports:

```cmake
NERD_BOARD_SOURCES
NERD_BOARD_INCLUDE_DIRS
NERD_BOARD_LEGACY_DIR
```

The RP2350 SoC wrapper exports:

```cmake
NERD_SOC_SOURCES
NERD_SOC_INCLUDE_DIRS
NERD_SOC_LEGACY_BSP_DIR
```

The legacy Pico backend now behaves differently by entry path:

```text
BUILD_CONFIG=pico2
  -> compile bsp/raspi-pico2/nd_hrtimer.c
  -> compile bsp/raspi-pico2/app.c
  -> compile bsp/raspi-pico2/nd_led.c
  -> compile bsp/raspi-pico2/shell_port.c

BOARD=pico2/rp2350a/m33
  -> arch-layer + soc-layer + board-layer + sample-layer + legacy-bsp
  -> compile soc/raspberrypi/rp2350/nd_hrtimer.c
  -> compile samples/pico_shell/src/app.c
  -> compile boards/raspberrypi/pico2/nd_led.c
  -> compile boards/raspberrypi/pico2/shell_port.c
```

Move or wrap files by responsibility.

Pico 2 examples:

```text
bsp/raspi-pico2/nd_led.c       -> boards/raspberrypi/pico2/
bsp/raspi-pico2/shell_port.c   -> boards/raspberrypi/pico2/
bsp/raspi-pico2/nd_hrtimer.c   -> soc/raspberrypi/rp2350/ or drivers/timer/
bsp/raspi-pico2/nerd.ld        -> boards/raspberrypi/pico2/nerd.ld now; split into board/soc linker fragments later
bsp/raspi-pico2/app.c          -> samples/pico_shell/src/app.c
```

Architecture examples:

```text
arch/arm/cortex_m/m33/*       -> arch/arm/cortex_m/m33/ or wrapped from there
arch/arm/a9/*        -> arch/arm/cortex_a/a9/ or wrapped from there
arch/riscv/rv32/*    -> arch/riscv/rv32/
```

Metadata aliases exported by `nerd_hw_metadata.h`:

```text
NERD_BOARD_MODEL / NERD_BOARD_COMPATIBLE
NERD_BOARD_CONSOLE_LABEL / NERD_BOARD_CONSOLE_BASE / NERD_BOARD_CONSOLE_IRQ / NERD_BOARD_CONSOLE_BAUD
NERD_SOC_SRAM_BASE / NERD_SOC_SRAM_SIZE
NERD_SOC_FLASH_BASE / NERD_SOC_FLASH_SIZE
NERD_SOC_TIMER_LABEL / NERD_SOC_TIMER_BASE / NERD_SOC_TIMER_IRQ / NERD_SOC_TIMER_CLOCK_FREQUENCY
```
Validation:

- Source movement is done one board or SoC group at a time.
- CMake configure passed for legacy `BUILD_CONFIG=pico2` with a temporary fake Pico SDK used only for configure-stage validation.
- CMake configure passed for `BOARD=pico2/rp2350a/m33` with a temporary fake Pico SDK used only for configure-stage validation.
- `build.ninja` confirms legacy `BUILD_CONFIG=pico2` still compiles `bsp/raspi-pico2/nd_hrtimer.c`, `bsp/raspi-pico2/app.c`, `bsp/raspi-pico2/nd_led.c`, and `bsp/raspi-pico2/shell_port.c`.
- `build.ninja` confirms `BOARD=pico2/rp2350a/m33` compiles `soc/raspberrypi/rp2350/nd_hrtimer.c`, `samples/pico_shell/src/app.c`, `boards/raspberrypi/pico2/nd_led.c`, and `boards/raspberrypi/pico2/shell_port.c`.
- `build.ninja` confirms legacy `BUILD_CONFIG=pico2` links with `bsp/raspi-pico2/nerd.ld`.
- `build.ninja` confirms `BOARD=pico2/rp2350a/m33` links with `boards/raspberrypi/pico2/nerd.ld`.
- Real build/link still requires a real `PICO_SDK_PATH`.
- Build output remains functionally equivalent after each move.
- Include paths are narrowed instead of globally expanded.

### Main Entry Decision

Decision: keep `main.c` as the last Pico BSP file to migrate.

Short-term target is option C: move each BSP `main.c` to its board layer only after the other board/soc/linker responsibilities are stable. Do not extract a generic kernel boot sequence yet, because Pico, QEMU A9, and STM32 all currently own their own `main.c` startup flow.

Rationale:

- Pico, QEMU A9, and STM32 startup code may differ in UART, timer, heap, interrupt, and scheduler setup details.
- Moving `main.c` too early would force a common init hook API before the platform differences are fully visible.
- Keeping `main.c` until the end preserves a working comparison path for each BSP.

Deferred work:

```text
bsp/raspi-pico2/main.c                  -> boards/raspberrypi/pico2/main.c later
bsp/vexpress-a9-qemu/main.c             -> boards/qemu/qemu_a9/main.c later
bsp/stm32f103rct6-m3-alientek/...main.c -> boards/st/stm32f103rct6_m3_alientek/main.c later
```

Only after those board-local moves are stable should NerdRTOS consider extracting a shared kernel boot sequence and platform init hooks.

## Step 6: Introduce Minimal DTS

Status: in progress. Minimal board DTS files and a lightweight DTS-to-header generator are present. The new `BOARD=...` CMake/Kconfig path generates `devicetree_generated.h` during configure, the DTS generator now emits both raw devicetree macros and a NerdRTOS board/soc metadata header. The Pico board console path consumes board metadata, and the RP2350 timer path consumes SoC metadata.

Added DTS files:

```text
boards/raspberrypi/pico2/pico2_rp2350a_m33.dts
boards/raspberrypi/pico2/pico2_rp2350a_hazard3.dts
boards/qemu/qemu_a9/qemu_a9.dts
boards/st/stm32f103rct6_m3_alientek/stm32f103rct6_m3_alientek.dts
```

Added generator:

```text
scripts/dts/gen_devicetree.py
```

Generated file location:

```text
<build>/kconfig/include/generated/devicetree_generated.h
<build>/kconfig/include/generated/nerd_hw_metadata.h
<build>/kconfig/include/generated/nerd_memory_regions.ld
```

Current generated raw hardware facts:

```text
DT_MODEL
DT_COMPATIBLE
DT_SRAM_BASE / DT_SRAM_SIZE
DT_FLASH_BASE / DT_FLASH_SIZE
DT_CONSOLE_LABEL / DT_CONSOLE_BASE / DT_CONSOLE_IRQ / DT_CONSOLE_BAUD
DT_TIMER_LABEL / DT_TIMER_BASE / DT_TIMER_IRQ / DT_TIMER_CLOCK_FREQUENCY
```

Metadata aliases exported by `nerd_hw_metadata.h`:

```text
NERD_BOARD_MODEL / NERD_BOARD_COMPATIBLE
NERD_BOARD_CONSOLE_LABEL / NERD_BOARD_CONSOLE_BASE / NERD_BOARD_CONSOLE_IRQ / NERD_BOARD_CONSOLE_BAUD
NERD_SOC_SRAM_BASE / NERD_SOC_SRAM_SIZE
NERD_SOC_FLASH_BASE / NERD_SOC_FLASH_SIZE
NERD_SOC_TIMER_LABEL / NERD_SOC_TIMER_BASE / NERD_SOC_TIMER_IRQ / NERD_SOC_TIMER_CLOCK_FREQUENCY
```
Validation:

- `BOARD=pico2/rp2350a/m33` configure generates `devicetree_generated.h` from `pico2_rp2350a_m33.dts`.
- `BOARD=pico2/rp2350a/hazard3` configure generates `devicetree_generated.h` from `pico2_rp2350a_hazard3.dts`, selects `CONFIG_ARCH_RISCV_RV32`, and sets `PICO_PLATFORM=rp2350-riscv`.
- Kconfig still merges board defconfig + `samples/pico_shell/prj.conf` + `samples/pico_shell/boards/<board>.conf`.
- Direct generator checks pass for `pico2_rp2350a_hazard3.dts`, `qemu_a9.dts`, and `stm32f103rct6_m3_alientek.dts`.
- `BOARD=qemu_a9/cortex_a9` and `BOARD=stm32f103rct6/m3` configure attempts now generate both DTS headers before reaching their legacy BSP toolchain checks.
- `boards/raspberrypi/pico2/shell_port.c` includes `nerd_hw_metadata.h`, uses `NERD_BOARD_CONSOLE_IRQ`, and checks `NERD_BOARD_CONSOLE_BASE` against the expected Pico UART0 base.
- Pico2 DTS console metadata now uses the RP2350 UART0 IRQ value `33`; using the old RP2040 value `20` allowed TX output but prevented RX interrupts and shell input.
- `soc/raspberrypi/rp2350/nd_hrtimer.c` includes `nerd_hw_metadata.h`, defines `ND_HRTIMER_CLOCK_HZ` from `NERD_SOC_TIMER_CLOCK_FREQUENCY`, and checks `NERD_SOC_TIMER_BASE` against the expected RP2350 timer base.

Remaining hardware facts to migrate from hardcoded C/linker values:

```text
console UART
system timer
SRAM/FLASH layout
CPU/timer clock frequency
```
## Step 7: Toolchain, Linker, Startup Layering

Status: completed for Pico2 M33 and Hazard3. QEMU A9 and STM32 remain deferred until Pico2 is complete as a sysbuild/domain shape. The first Pico2 linker split is complete: DTS generation now emits `nerd_memory_regions.ld`, and `boards/raspberrypi/pico2/nerd.ld` includes that generated MEMORY fragment instead of hardcoding RAM and scratch regions directly. Pico2 architecture source ownership has also moved into the arch wrappers: `arch/arm/cortex_m` exports the M33 sources, and `arch/riscv/rv32` exports the Hazard3 RV32 sources. Pico2 pre-project SDK board/platform metadata now lives in `boards/raspberrypi/pico2/pre_cmake.cmake`, so the top-level CMake no longer hardcodes Pico SDK board/platform choices directly. Pico2 board wrapper now exports SDK target metadata such as system include dirs, printf implementation, extra outputs, and SDK libraries; the legacy backend consumes those variables with fallback defaults. Pico2 configure now also writes `build_layers.txt`, which records the selected board, arch/soc/board/sample layers, backend sources, linker script/fragments, DTS outputs, Pico SDK target metadata, and arch/toolchain metadata. For Pico2, board-target builds identify the toolchain backend as `pico_sdk`; M33 records the expected `-mcpu=cortex-m33 -mthumb` arch options, while Hazard3 records `PICO_BOARD=pico2` and `PICO_PLATFORM=rp2350-riscv` and leaves CPU flags to the Pico SDK platform backend. Startup entry ownership is now explicit metadata: the current entry implementation still comes from `bsp/raspi-pico2/main.c`, the backend is `legacy_bsp`, and the target owner is the board layer.

Move compile/link/startup ownership to the correct layer:

```text
arch: compiler CPU flags, startup, trap/context, arch linker defaults
soc: SoC memory layout, vector placement, interrupt/timer integration
board: board linker aliases, bootloader offsets, flash layout, runner config
```

Metadata aliases exported by `nerd_hw_metadata.h`:

```text
NERD_BOARD_MODEL / NERD_BOARD_COMPATIBLE
NERD_BOARD_CONSOLE_LABEL / NERD_BOARD_CONSOLE_BASE / NERD_BOARD_CONSOLE_IRQ / NERD_BOARD_CONSOLE_BAUD
NERD_SOC_SRAM_BASE / NERD_SOC_SRAM_SIZE
NERD_SOC_FLASH_BASE / NERD_SOC_FLASH_SIZE
NERD_SOC_TIMER_LABEL / NERD_SOC_TIMER_BASE / NERD_SOC_TIMER_IRQ / NERD_SOC_TIMER_CLOCK_FREQUENCY
```
Validation:

- `BOARD=pico2/rp2350a/m33` configure generates `nerd_memory_regions.ld` from DTS. The generated MEMORY fragment preserves the previous Pico2 layout: `RAM` at `0x20000000` with `0x80000` bytes, `SCRATCH_X` at `0x20080000`, and `SCRATCH_Y` at `0x20081000`.
- `BOARD=pico2/rp2350a/hazard3` configure generates the same Pico2 MEMORY fragment and still selects the RISC-V arch layer.
- Pico2 BOARD builds add the generated directory to linker search paths with `-Wl,-L<build>/kconfig/include/generated`.
- `BOARD=pico2/rp2350a/m33` now gets `NERD_ARCH_SOURCES` from `arch/arm/cortex_m/CMakeLists.txt`.
- `BOARD=pico2/rp2350a/hazard3` now gets `NERD_ARCH_SOURCES` and `NERD_ARCH_INCLUDE_DIRS` from `arch/riscv/rv32/CMakeLists.txt`.
- Legacy `BUILD_CONFIG=pico2` still configures because `cmake/nerd_arch.cmake` remains as a fallback when no arch wrapper has exported sources.
- `BOARD=pico2/rp2350a/hazard3` gets `PICO_BOARD=pico2` and `PICO_PLATFORM=rp2350-riscv` from `boards/raspberrypi/pico2/pre_cmake.cmake`, before Pico SDK import and `project()`.
- `BOARD=pico2/rp2350a/m33` sets `PICO_BOARD=pico2` before Pico SDK import so the real SDK selects RP2350 instead of its default `pico`/RP2040 board.
- Pico2 BOARD paths get `NERD_PICO_SDK_SYSTEM_INCLUDE_DIRS`, `NERD_PICO_PRINTF_IMPLEMENTATION`, `NERD_PICO_EXTRA_OUTPUTS`, and `NERD_PICO_LIBRARIES` from `boards/raspberrypi/pico2/CMakeLists.txt`.
- Legacy `BUILD_CONFIG=pico2` still configures because `bsp/raspi-pico2/CMakeLists.txt` provides fallback defaults for the same Pico SDK target metadata.
- `BOARD=pico2/rp2350a/m33` writes `build_layers.txt` with `ARCH_LAYER=cortex_m`, `BSP_SOURCES=main.c`, the board-owned linker script, the generated linker memory fragment, DTS headers, and Pico SDK target metadata.
- `BOARD=pico2/rp2350a/hazard3` writes `build_layers.txt` with `ARCH_LAYER=rv32`, `PICO_BOARD=pico2`, `PICO_PLATFORM=rp2350-riscv`, the board-owned linker script, the generated linker memory fragment, DTS headers, and Pico SDK target metadata.
- Legacy `BUILD_CONFIG=pico2` writes `build_layers.txt` with the legacy source list and legacy linker script, while leaving DTS/linker-fragment fields empty because legacy does not use the new DTS path.
- Pico2 `build_layers.txt` records `TOOLCHAIN_BACKEND=pico_sdk` for both board-target and legacy entry paths.
- `BOARD=pico2/rp2350a/m33` records `ARCH_CPU=cortex-m33`, `ARCH_INSTRUCTION_SET=thumb`, and expected arch compile options `-mcpu=cortex-m33;-mthumb`.
- `BOARD=pico2/rp2350a/hazard3` records `ARCH_CPU=hazard3`, `ARCH_INSTRUCTION_SET=rv32`, and relies on the Pico SDK `rp2350-riscv` platform backend for concrete CPU flags.
- Pico2 M33, Hazard3, and legacy entry paths all record `STARTUP_BACKEND=legacy_bsp`, `STARTUP_OWNER=board`, and `ENTRY_SOURCE=bsp/raspi-pico2/main.c`; `main.c` remains intentionally unmoved.
- Step7 closeout configure matrix passed for `BOARD=pico2/rp2350a/m33`, `BOARD=pico2/rp2350a/hazard3`, and legacy `BUILD_CONFIG=pico2` with a temporary fake Pico SDK.
- Step7 closeout confirmed `BOARD=pico2/rp2350a/m33` generates `.config`, `autoconf.h`, DTS headers, `nerd_memory_regions.ld`, `build_layers.txt`, and `build.ninja`; `build.ninja` uses M33 arch sources, RP2350 SoC timer source, Pico2 board sources, sample source, legacy `main.c`, and the board-owned linker script.
- Step7 closeout confirmed `BOARD=pico2/rp2350a/hazard3` generates the same output set and uses RV32 arch sources, RP2350 SoC timer source, Pico2 board sources, sample source, legacy `main.c`, `PICO_PLATFORM=rp2350-riscv`, and the board-owned linker script.
- Step7 closeout confirmed legacy `BUILD_CONFIG=pico2` keeps the old BSP source list and legacy linker script, and intentionally does not generate DTS headers or the generated linker memory fragment.
- Real Pico SDK/toolchain build passed for `BOARD=pico2/rp2350a/m33`; `PICO_BOARD=pico2`, `PICO_PLATFORM=rp2350-arm-s`, `PICO_RP2350=1`, and `PICO_RP2040=0` were confirmed from `CMakeCache.txt`.
- Real Pico SDK/toolchain build passed for `BOARD=pico2/rp2350a/hazard3`; `PICO_BOARD=pico2` and `PICO_PLATFORM=rp2350-riscv` were confirmed from generated metadata.
- Real linker maps for M33 and Hazard3 both use the generated RP2350 memory layout: `FLASH` at `0x10000000`, `RAM` at `0x20000000`, `SCRATCH_X` at `0x20080000`, and `SCRATCH_Y` at `0x20081000`.
- M33 and Hazard3 UF2 images were flashed to Pico2 and both booted successfully.
- Serial shell runtime validation passed after correcting the Pico2 DTS console IRQ from the RP2040 value `20` to the RP2350 UART0 IRQ value `33`; shell input and command handling are functional.
- Cross-toolchain builds still use the correct compiler and flags.
- Linker scripts are not duplicated across boards unnecessarily.
- Cortex-M33 implementation files now live under `arch/arm/cortex_m/m33`; `arch/arm/a9` and `arch/arm/m3` remain deferred until their board migrations.

## Step 8: Add sysbuild/domain

Status: completed for Pico2 M33 and Hazard3. A top-level `sysbuild/` CMake entry manages the two Pico2 domains without changing the single-domain application build. QEMU A9 and STM32 remain deferred.

Initial enabled domains:

```text
pico2_m33      -> BOARD=pico2/rp2350a/m33
pico2_hazard3  -> BOARD=pico2/rp2350a/hazard3
```

Sysbuild entry:

```text
sysbuild/CMakeLists.txt
```

The sysbuild entry uses CMake `ExternalProject` to create independent domain build directories:

```text
<sysbuild-build>/domains/pico2_m33
<sysbuild-build>/domains/pico2_hazard3
```

Each domain still runs the normal NerdRTOS root build with its own `BOARD=...` value. The sysbuild layer only orchestrates domains and writes metadata.

Basic commands:

```bash
cmake -S sysbuild -B build-sys-pico2 -G Ninja -DNERD_DOMAIN_PICO_SDK_PATH="$PICO_SDK_PATH"
cmake --build build-sys-pico2 --target domains_configure
cmake --build build-sys-pico2 --target domains
```

Per-domain helper targets:

```bash
cmake --build build-sys-pico2 --target pico2_m33_menuconfig
cmake --build build-sys-pico2 --target pico2_hazard3_menuconfig
cmake --build build-sys-pico2 --target pico2_m33_guiconfig
cmake --build build-sys-pico2 --target pico2_hazard3_guiconfig
```

Generated sysbuild metadata:

```text
<sysbuild-build>/domains.txt
<sysbuild-build>/domains.yaml
```

Metadata aliases exported by `nerd_hw_metadata.h`:

```text
NERD_BOARD_MODEL / NERD_BOARD_COMPATIBLE
NERD_BOARD_CONSOLE_LABEL / NERD_BOARD_CONSOLE_BASE / NERD_BOARD_CONSOLE_IRQ / NERD_BOARD_CONSOLE_BAUD
NERD_SOC_SRAM_BASE / NERD_SOC_SRAM_SIZE
NERD_SOC_FLASH_BASE / NERD_SOC_FLASH_SIZE
NERD_SOC_TIMER_LABEL / NERD_SOC_TIMER_BASE / NERD_SOC_TIMER_IRQ / NERD_SOC_TIMER_CLOCK_FREQUENCY
```
Validation:

- Sysbuild configure passed with a temporary fake Pico SDK.
- `domains_configure` configures both Pico2 domains without compiling them.
- Sysbuild metadata lists `pico2_m33` and `pico2_hazard3` in `domains.txt` and `domains.yaml`.
- `pico2_m33` has its own `.config`, `autoconf.h`, DTS headers, `nerd_memory_regions.ld`, `build_layers.txt`, and `build.ninja` under `<sysbuild-build>/domains/pico2_m33`.
- `pico2_hazard3` has its own `.config`, `autoconf.h`, DTS headers, `nerd_memory_regions.ld`, `build_layers.txt`, and `build.ninja` under `<sysbuild-build>/domains/pico2_hazard3`.
- `pico2_m33/build_layers.txt` records `BOARD=pico2/rp2350a/m33`, `ARCH_LAYER=cortex_m`, `ARCH_CPU=cortex-m33`, `PICO_BOARD=pico2`, and the domain-local generated linker memory fragment.
- `pico2_hazard3/build_layers.txt` records `BOARD=pico2/rp2350a/hazard3`, `ARCH_LAYER=rv32`, `ARCH_CPU=hazard3`, `PICO_BOARD=pico2`, `PICO_PLATFORM=rp2350-riscv`, and the domain-local generated linker memory fragment.
- Real sysbuild compilation with the real Pico SDK/toolchain passed for both `pico2_m33` and `pico2_hazard3`.
- Each Pico2 domain produces its own independent ELF/UF2 artifacts under its domain build directory.
- No UF2 merge/package step is added for now; M33 and Hazard3 UF2 files intentionally remain independent because they are owned/flashed separately.

## Step 9: Clean Up Legacy Entry Points

Status: completed for Pico2 cleanup, except for the intentionally deferred physical `main.c` move. Pico2 top-level `BUILD_CONFIG=pico2` and standalone BSP entry usage are retired. The BSP backend is still kept as the executable/startup backend for board-target builds, but it no longer falls back to BSP-owned timer/app/board source copies. Stale BSP duplicate source/header files, legacy Pico2 defconfigs, and the old BSP linker script have been deleted after M33, Hazard3, and sysbuild verification.

Retired compatibility entry points:

```text
BUILD_CONFIG=pico2
configs/pico2_defconfig
configs/pico2_arm_defconfig
configs/pico2_riscv_defconfig
bsp/raspi-pico2 standalone CMake entry
```

Backend files still kept for board-target builds:

```text
bsp/raspi-pico2/CMakeLists.txt
bsp/raspi-pico2/main.c
bsp/raspi-pico2/pico_sdk_import.cmake
```

Duplicated Pico2 files cleaned up after board/soc/sample ownership was proven without fallback:

```text
bsp/raspi-pico2/nd_led.c       -> boards/raspberrypi/pico2/nd_led.c
bsp/raspi-pico2/nd_led.h       -> boards/raspberrypi/pico2/nd_led.h
bsp/raspi-pico2/shell_port.c   -> boards/raspberrypi/pico2/shell_port.c
bsp/raspi-pico2/nd_hrtimer.c   -> soc/raspberrypi/rp2350/nd_hrtimer.c
bsp/raspi-pico2/app.c          -> samples/pico_shell/src/app.c
bsp/raspi-pico2/app.h          -> samples/pico_shell/include/app.h
bsp/raspi-pico2/nerd.ld        -> boards/raspberrypi/pico2/nerd.ld
```

Deferred startup cleanup:

```text
bsp/raspi-pico2/main.c -> boards/raspberrypi/pico2/main.c later
```

`main.c` remains intentionally deferred because QEMU A9 and STM32 also own their startup entry flows. Move it only after the board-local startup pattern is clear across BSPs.

Documentation/tooling cleanup candidates:

```text
bsp/raspi-pico2/README.md
.vscode/tasks.json or equivalent local build tasks
README build commands
```

Step9 cleanup execution table:

| Area | Current board-target owner | Legacy dependency still present | Cleanup rule | Verification before removal |
| --- | --- | --- | --- | --- |
| `app.c` / `app.h` | `samples/pico_shell/src/app.c`, `samples/pico_shell/include/app.h` | No active top-level fallback remains after Step9C. | Deleted from `bsp/raspi-pico2/` after M33/Hazard3/sysbuild verification. | Source search shows no active code references remain. |
| `nd_led.c` / `nd_led.h` / `shell_port.c` | `boards/raspberrypi/pico2/` | No active top-level fallback remains after Step9C. | Deleted from `bsp/raspi-pico2/` after M33/Hazard3/sysbuild verification. | Source search shows no active code references remain. |
| `nd_hrtimer.c` | `soc/raspberrypi/rp2350/nd_hrtimer.c` | No active top-level fallback remains after Step9C. | Deleted from `bsp/raspi-pico2/` after M33/Hazard3/sysbuild verification. | Source search shows no active code references remain. |
| `nerd.ld` | `boards/raspberrypi/pico2/nerd.ld` plus generated `nerd_memory_regions.ld` | No active top-level legacy linker path remains after retiring `BUILD_CONFIG=pico2`. | Deleted from `bsp/raspi-pico2/` after source search confirmed no active tooling references remained. | M33/Hazard3 map files use the board-owned linker script and generated memory fragment. |
| `main.c` | Still `bsp/raspi-pico2/main.c`; Pico2 board metadata exports it through `NERD_ENTRY_SOURCE`. | The BSP backend now compiles `NERD_ENTRY_SOURCE`, so the path is no longer hardcoded in the backend. | Defer the physical move. Later move to `boards/raspberrypi/pico2/main.c` and update only the board wrapper path. | Boot banner, shell prompt, timer, scheduler, and command handling still work after the later move. |
| `pico_sdk_import.cmake` | Pico board pre-project metadata lives in `boards/raspberrypi/pico2/pre_cmake.cmake`; SDK import is still used by the top-level Pico board path. | The file location is still under `bsp/raspi-pico2`, even though standalone BSP entry usage is retired. | Keep for now. Later move SDK import policy to a shared CMake module or board backend helper. | Clean configure from an empty build directory for both Pico2 domains. |
| `configs/pico2*.defconfig` | Board-target defconfigs live in `boards/raspberrypi/pico2/`. | No active top-level Pico2 path depends on `configs/pico2*.defconfig` after Step9B. | Deleted after source search confirmed no docs/tasks/scripts still advertise it as an active build path. | Replacement `BOARD=...` commands are documented and validated. |
| `bsp/raspi-pico2/CMakeLists.txt` | Board/soc/sample wrappers now export most source ownership. | It is still the final executable backend and still owns `main.c`, Pico SDK target wiring fallbacks, and metadata emission. | Shrink only after source fallbacks are gone. Remove only after executable creation/startup ownership moves out. | `build_layers.txt` still records the intended layer ownership after each shrink. |
| `bsp/raspi-pico2/cmake/check_config.cmake` | Some validation now belongs to board/pre-CMake metadata and Kconfig board targets. | The legacy backend still uses this file to guard invalid Pico SDK platform combinations. | Keep until validation is centralized in board metadata or top-level board-target checks. | Invalid Hazard3-with-ARM-platform configurations still fail early and clearly. |
| README / VSCode tasks | New commands are board-target and sysbuild based. | Local tooling no longer points at retired `BUILD_CONFIG=pico2` or standalone `bsp/raspi-pico2`. | Completed: README, BSP README, VSCode settings, VSCode tasks, and IntelliSense include paths use explicit `BOARD=...`, sysbuild, and new source ownership. | A new user can follow the documented M33, Hazard3, and sysbuild commands from a clean clone. |

Recommended Step9 execution order:

1. Step9A: completed. The legacy fallback behavior in `bsp/raspi-pico2/CMakeLists.txt` is now split into explicit entry, SoC fallback, sample fallback, and board fallback source groups.
2. Step9B: completed. `BUILD_CONFIG=pico2` is deliberately retired in the top-level build; users must pass `BOARD=pico2/rp2350a/m33` or `BOARD=pico2/rp2350a/hazard3` explicitly.
3. Step9C: completed. BSP fallback source groups are disabled; `bsp/raspi-pico2/CMakeLists.txt` now requires board/soc/sample layers and only contributes `main.c`.
4. Step9D: in progress. Startup source indirection is added: the Pico2 backend now compiles `NERD_ENTRY_SOURCE` instead of hardcoding `main.c`; the actual file move remains deferred.
5. Step9E: completed. README, Pico2 BSP README, VSCode CMake source directory, VSCode tasks, and VSCode IntelliSense include paths now match the explicit `BOARD=...` and sysbuild flows instead of retired `BUILD_CONFIG=pico2` or standalone BSP commands.




Step9 stale duplicate cleanup completed:

```text
removed from bsp/raspi-pico2/:
  app.c
  app.h
  nd_led.c
  nd_led.h
  shell_port.c
  nd_hrtimer.c

kept in bsp/raspi-pico2/:
  CMakeLists.txt
  Kconfig
  main.c
  nd_config.h
  pico_sdk_import.cmake
  README.md
```

The removed files are now owned by `samples/pico_shell/`, `boards/raspberrypi/pico2/`, and `soc/raspberrypi/rp2350/`.

Step9 legacy config/linker cleanup completed:

```text
removed:
  configs/pico2_defconfig
  configs/pico2_arm_defconfig
  configs/pico2_riscv_defconfig
  bsp/raspi-pico2/nerd.ld

active replacements:
  boards/raspberrypi/pico2/pico2_rp2350a_m33_defconfig
  boards/raspberrypi/pico2/pico2_rp2350a_hazard3_defconfig
  boards/raspberrypi/pico2/nerd.ld
  <build>/kconfig/include/generated/nerd_memory_regions.ld
```
Step9D startup ownership plan:

```text
current:
  boards/raspberrypi/pico2/CMakeLists.txt
    -> NERD_ENTRY_SOURCE=${PROJECT_ROOT}/bsp/raspi-pico2/main.c
  bsp/raspi-pico2/CMakeLists.txt
    -> add_executable(... ${NERD_ENTRY_SOURCE} ...)

later:
  boards/raspberrypi/pico2/main.c
  boards/raspberrypi/pico2/CMakeLists.txt
    -> NERD_ENTRY_SOURCE=${CMAKE_CURRENT_LIST_DIR}/main.c
```

This keeps the current Pico2 boot behavior unchanged while removing the backend's hardcoded dependency on `bsp/raspi-pico2/main.c`. The physical move is intentionally deferred because QEMU A9 and STM32 still have board-specific startup flows that should inform the final common pattern.

Step9E documentation/tooling cleanup completed:

```text
updated:
  README.md
  bsp/raspi-pico2/README.md
  .vscode/settings.json
  .vscode/tasks.json
  .vscode/c_cpp_properties.json

advertised flows:
  BOARD=pico2/rp2350a/m33
  BOARD=pico2/rp2350a/hazard3
  sysbuild -> pico2_m33 + pico2_hazard3
```
Step9B decision:

```text
BUILD_CONFIG=pico2         -> retired
BOARD=pico2/rp2350a/m33    -> supported Pico2 ARM target
BOARD=pico2/rp2350a/hazard3 -> supported Pico2 RISC-V target
```

The top-level build no longer defaults to `BUILD_CONFIG=pico2` when neither `BOARD` nor `BUILD_CONFIG` is provided. This removes the ambiguity of plain `pico2`, because Pico2 now has two explicit CPU domains.

Step9 verification commands:

```bash
cmake -S . -B build-pico2-m33 -G Ninja -DBOARD=pico2/rp2350a/m33 -DPICO_SDK_PATH="$PICO_SDK_PATH"
cmake --build build-pico2-m33
cmake -S . -B build-pico2-hazard3 -G Ninja -DBOARD=pico2/rp2350a/hazard3 -DPICO_SDK_PATH="$PICO_SDK_PATH"
cmake --build build-pico2-hazard3
cmake -S sysbuild -B build-sys-pico2 -G Ninja -DNERD_DOMAIN_PICO_SDK_PATH="$PICO_SDK_PATH"
cmake --build build-sys-pico2 --target domains
```

Legacy `BUILD_CONFIG=pico2` is retired. Verify that it now fails with a clear replacement message:

```bash
cmake -S . -B build-pico2-legacy-retired -G Ninja -DBUILD_CONFIG=pico2
```
Step9 cleanup prerequisites:

- Board-target builds remain green for `pico2/rp2350a/m33` and `pico2/rp2350a/hazard3`.
- Sysbuild `domains` target remains green for `pico2_m33` and `pico2_hazard3`.
- Legacy `BUILD_CONFIG=pico2` is deliberately retired in the top-level build, with replacement `BOARD=...` commands documented.
- `main.c` ownership has a separate migration decision.

Things explicitly not planned now:

- Do not merge the M33 and Hazard3 UF2 files.
- Do not delete the legacy Pico2 BSP backend yet.
- Do not start QEMU A9 or STM32 cleanup before Pico2 cleanup rules are stable.

Metadata aliases exported by `nerd_hw_metadata.h`:

```text
NERD_BOARD_MODEL / NERD_BOARD_COMPATIBLE
NERD_BOARD_CONSOLE_LABEL / NERD_BOARD_CONSOLE_BASE / NERD_BOARD_CONSOLE_IRQ / NERD_BOARD_CONSOLE_BAUD
NERD_SOC_SRAM_BASE / NERD_SOC_SRAM_SIZE
NERD_SOC_FLASH_BASE / NERD_SOC_FLASH_SIZE
NERD_SOC_TIMER_LABEL / NERD_SOC_TIMER_BASE / NERD_SOC_TIMER_IRQ / NERD_SOC_TIMER_CLOCK_FREQUENCY
```
Validation:

- Cleanup plan identifies every Pico2 legacy file that is currently duplicated by board/soc/sample/linker ownership.
- No files are removed in the planning step.
- All documented commands still work from a clean clone before any cleanup is attempted.


### Arch Layout Cleanup

Status: completed for Pico2 M33 only. The Cortex-M33 implementation has moved from the old flat CPU directory into the Cortex-M family directory:

```text
arch/arm/m33/context.S    -> arch/arm/cortex_m/m33/context.S
arch/arm/m33/port.c       -> arch/arm/cortex_m/m33/port.c
arch/arm/m33/hardfault.c  -> arch/arm/cortex_m/m33/hardfault.c
arch/arm/m33/systick.c    -> arch/arm/cortex_m/m33/systick.c
```

Updated references:

```text
arch/arm/cortex_m/CMakeLists.txt
cmake/nerd_arch.cmake
```

Deferred arch cleanup:

```text
arch/arm/a9 -> arch/arm/cortex_a/a9 later, when QEMU A9 migration resumes
arch/arm/m3 -> arch/arm/cortex_m/m3 later, when STM32 migration starts
```

Rationale: Pico2 M33 has already passed board-target and sysbuild validation, so it is safe to move the M33 implementation now. A9 and M3 still have standalone BSP dependencies and should remain in their old paths until their board-target migrations are active.
## Step 10: Main-tree Verification Matrix

Each step should update this matrix.

| Target | Configure | Build | Run/Flash | Notes |
| --- | --- | --- | --- | --- |
| pico2/rp2350a/m33 | pass with real Pico SDK | pass | pass | primary Pico 2 ARM target; flashed and shell validated |
| pico2/rp2350a/hazard3 | pass with real Pico SDK | pass | pass | Pico 2 RISC-V target; `PICO_PLATFORM=rp2350-riscv`; flashed and shell validated |
| qemu_a9/cortex_a9 | partial: Kconfig + DTS pass, toolchain missing | pending | pending | deferred until Pico2 is complete |
| stm32f103rct6/m3 | partial: Kconfig + DTS pass, toolchain missing | pending | pending | deferred until Pico2 is complete |

## Immediate Next Action

Continue Pico2 first. Step8 sysbuild/domain is complete for Pico2 M33 and Hazard3 with real build validation. The next concrete task is final Pico2 Step9 verification: run M33, Hazard3, sysbuild, and the retired legacy command check once more. After that, move on to the next board family; the physical `main.c` move remains deferred until startup ownership is finalized across Pico2, QEMU A9, and STM32.
