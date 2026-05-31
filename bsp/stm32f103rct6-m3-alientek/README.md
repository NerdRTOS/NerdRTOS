# NerdRTOS — STM32F103RCT6 (Alientek Mini) BSP

Board Support Package for running NerdRTOS on the **STM32F103RCT6**
(Alientek Mini development board), Cortex-M3 (ARMv7-M).

---

## Hardware

| Item | Detail |
|------|--------|
| MCU | STM32F103RCT6 |
| Core | ARM Cortex-M3 @ 72 MHz |
| Flash | 256 KB |
| SRAM | 48 KB |
| Debug Probe | ST-Link (SWD) |
| UART | USART1: TX = PA9, RX = PA10, 115200-8N1 |

---

## Features

This demo showcases a fully functional **priority-based preemptive RTOS**
with an interactive shell running over UART:

- **LED Blink Thread** — priority 5, 512 B stack, toggles LED0 every 500 ms
- **Interactive Shell** — priority 30, 1024 B stack, accessible via serial terminal
- **Shell Commands:**
  - `ps` — list all threads with state, priority, stack usage, and CPU load
  - `help` — list all available commands
  - `load` — display CPU load percentage
- **In-Tree Test Framework (NTest):**
  - `test_sem` — semaphore test suite
  - `test_memheap` — memory heap test suite
  - `test_mempool` — memory pool test suite
  - `test_thread_lifecycle` — thread lifecycle test suite

---

## Prerequisites

### Required Software

- **ARM GNU Toolchain** (`arm-none-eabi-gcc`) — tested with 13.3.rel1
- **CMake** 3.13+
- **OpenOCD** — for flashing and debugging via ST-Link
- **VS Code** (recommended) — with *Cortex-Debug* and *CMake Tools* extensions

### STM32 Vendor Files (not in the repository)

The STM32 HAL and CMSIS files are **not included** in the repository
(gitignored due to size) and must be obtained separately from the
[STM32CubeF1](https://github.com/STMicroelectronics/STM32CubeF1) firmware
package or generated with STM32CubeMX.

Create the following directories under `bsp/stm32f103rct6-m3-alientek/`:

```
Drivers/
├── CMSIS/
│   ├── Include/
│   │   ├── core_cm3.h
│   │   ├── cmsis_gcc.h
│   │   └── ...
│   └── Device/ST/STM32F1xx/
│       ├── Include/
│       │   ├── stm32f103xe.h
│       │   └── ...
│       └── Source/Templates/gcc/
│           ├── startup_stm32f103xe.s
│           └── system_stm32f1xx.c
└── STM32F1xx_HAL_Driver/
    ├── Inc/
    │   ├── stm32f1xx_hal.h
    │   ├── stm32f1xx_hal_uart.h
    │   └── ...
    └── Src/
        ├── stm32f1xx_hal.c
        ├── stm32f1xx_hal_uart.c
        └── ...
```

### OpenOCD Configuration

The file `openocd.cfg` contains paths to OpenOCD interface/target scripts.
Adjust the paths to match your OpenOCD installation:

```tcl
# Default (Windows-style):
source D:/openocd/openocd/scripts/interface/stlink.cfg
source D:/openocd/openocd/scripts/target/stm32f1x.cfg
```

---

## Build

### Command Line

```bash
cmake -S bsp/stm32f103rct6-m3-alientek \
      -B build/stm32f103rct6-m3-alientek \
      -DCMAKE_TOOLCHAIN_FILE=bsp/stm32f103rct6-m3-alientek/toolchain-arm-none-eabi.cmake \
      -DCMAKE_BUILD_TYPE=Debug

cmake --build build/stm32f103rct6-m3-alientek
```

Build artifacts (`Firmware.elf`, `.hex`, `.bin`, `.map`) are placed in
`bsp/stm32f103rct6-m3-alientek/Outputs/`.

### VS Code

Open the repo root in VS Code. The pre-configured tasks and launch
settings in `.vscode/` provide one-click build and debug:

- **Build:** `Ctrl+Shift+B` → select the CMake task
- **Debug:** `F5` → launches OpenOCD + GDB, halts at `main()`

---

## Flash and Debug

### Flash with OpenOCD

```bash
openocd -f bsp/stm32f103rct6-m3-alientek/openocd.cfg \
        -c "program bsp/stm32f103rct6-m3-alientek/Outputs/Firmware.elf verify reset exit"
```

### Debug with GDB

```bash
# Terminal 1 — start OpenOCD
openocd -f bsp/stm32f103rct6-m3-alientek/openocd.cfg

# Terminal 2 — connect GDB
arm-none-eabi-gdb bsp/stm32f103rct6-m3-alientek/Outputs/Firmware.elf \
    -ex "target extended-remote :3333" \
    -ex "monitor reset init" \
    -ex "continue"
```

### VS Code Cortex-Debug

The `.vscode/launch.json` includes a pre-configured debug session
**"STM32F103 Debug (OpenOCD)"** that:
- Builds via CMake task before launching
- Starts OpenOCD with the local `openocd.cfg`
- Connects GDB and breaks at `main()`

---

## Serial Terminal

Connect to the board at **115200 baud, 8N1**:

```
Hello Nerd RTOS!
nerd@rtos:~$ help

 - test_memheap - memheap test suite
 - test_mempool - mempool test suite
 - test_sem - semaphore test suite
 - test_thread_lifecycle - thread lifecycle test suite
 - load - CPU load
 - ps - Thread information
 - help - list command

nerd@rtos:~$ ps

CPU Load: 3%
Name       State      Pri   Stack    Used   Load
---------- -------- ----- ------- ------- ------
idle       READY       31     512      44    97%
led_blink  BLOCK        5     512     100     0%
shell      RUN         30    1024     392     2%
nerd@rtos:~$
```

> **Note:** Some serial terminals append both `\r` and `\n` on enter,
> which causes an extra blank prompt line. This is cosmetic and does
> not affect functionality.

---

## Configuration (`Users/nd_config.h`)

| Macro | Value | Description |
|-------|-------|-------------|
| `ND_CPU_CLOCK_HZ` | `64000000UL` | CPU core clock (HSI/PLL) |
| `ND_TICKS_PER_SEC` | `1000` | System tick rate |
| `ND_CFG_TICKLESS` | `N` | Tick mode (periodic SysTick) |
| `ND_CFG_DEBUG` | `N` | Debug output disabled |
| `ND_IDLE_STACK_SIZE` | `512` | Idle thread stack size |
| `ND_NAME_MAX_SIZE` | `16` | Thread name max length |

---

## Project Structure

```
bsp/stm32f103rct6-m3-alientek/
├── CMakeLists.txt                   # Top-level CMake (MCU flags, subdirs)
├── toolchain-arm-none-eabi.cmake    # Cross-compiler toolchain
├── stm32f103rct6.ld                 # Linker script (256K flash, 48K RAM)
├── openocd.cfg                      # OpenOCD config (ST-Link + STM32F1x)
├── Outputs/                         # Build artifacts (.elf, .hex, .bin)
├── Drivers/
│   ├── CMSIS/                       # CMSIS headers + startup (vendor)
│   ├── STM32F1xx_HAL_Driver/        # HAL library (vendor)
│   └── BSP/
│       ├── led/                     # LED toggle driver
│       └── usart/                   # USART1 driver (interrupt RX, direct TX)
└── Users/
    ├── CMakeLists.txt               # Application build targets
    ├── main.c                       # Entry point, bsp_init, kernel startup
    ├── app.c / app.h                # Thread creation
    ├── shell_port.c                 # Shell backend (UART I/O, ring buffer)
    ├── nd_config.h                  # RTOS configuration
    └── stm32f1xx_hal_conf.h        # HAL module selection
```

The application references NerdRTOS kernel, arch (Cortex-M3), components
(shell, ntest), and library (rbtree) by **relative path** from the repo root.

---

## Kernel Startup Flow

```
main()
├── nd_kernel_lock()            # Disable interrupts globally
├── HAL_Init()                  # Init HAL tick
├── system_clock_init()         # 72 MHz HSI/PLL
├── usart_init(115200)          # USART1: TX polling, RX interrupt
├── nd_hw_tick_init()           # SysTick @ 1000 Hz
├── nd_system_heap_init()       # Heap from linker symbols
├── nd_led_init()               # LED GPIO
├── uart_puts("Hello Nerd RTOS!\r\n")  # Boot message
├── nd_scheduler_init()         # Idle thread + ready lists
├── nd_app_init()               # Create led_blink + shell threads
└── nd_scheduler_start()        # First context switch → never returns
```

---

## Debugging Tips

### HardFault Handler

The Cortex-M3 HardFault handler at `arch/arm/m3/hardfault.c` dumps
all relevant registers (R0-R3, R12, LR, PC, PSR, CFSR, HFSR, MMFAR,
BFAR) when a fault occurs. Connect via GDB and the output will appear
in the debug console.

### Stack Usage Monitoring

`ps` reports per-thread stack usage by scanning the 0xAA watermark
placed by the thread creation code. If usage approaches the stack
size, increase it in `app.c`.

### Build Warnings

```bash
cmake --build build/stm32f103rct6-m3-alientek 2>&1 | tee build.log
python3 .github/scripts/check_build.py build.log
```

### Style Check

```bash
python3 .github/scripts/check_style.py
```
