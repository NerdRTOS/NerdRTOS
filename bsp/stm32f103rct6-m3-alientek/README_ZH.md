# NerdRTOS — STM32F103RCT6（正点原子 Mini）BSP

基于 **STM32F103RCT6**（正点原子 Mini 开发板）的 NerdRTOS 板级支持包，
CPU 架构为 Cortex-M3 (ARMv7-M)。

---

## 硬件信息

| 项目 | 参数 |
|------|------|
| MCU | STM32F103RCT6 |
| 内核 | ARM Cortex-M3 @ 72 MHz |
| Flash | 256 KB |
| SRAM | 48 KB |
| 调试器 | ST-Link (SWD) |
| 串口 | USART1：TX = PA9, RX = PA10, 115200-8N1 |

---

## 功能介绍

本示例演示了一个**基于优先级抢占的完整 RTOS**，并通过 UART 提供交互式 Shell：

- **LED 闪烁线程** — 优先级 5，栈大小 512 字节，每 500 ms 翻转 LED0
- **交互式 Shell** — 优先级 30，栈大小 1024 字节，通过串口终端访问
- **Shell 命令：**
  - `ps` — 列出所有线程的状态、优先级、栈使用量和 CPU 负载
  - `help` — 列出所有可用命令
  - `load` — 显示 CPU 负载百分比
- **内置测试框架 (NTest)：**
  - `test_sem` — 信号量测试套件
  - `test_memheap` — 内存堆测试套件
  - `test_mempool` — 内存池测试套件
  - `test_thread_lifecycle` — 线程生命周期测试套件

---

## 前置条件

### 必要软件

- **ARM GNU Toolchain** (`arm-none-eabi-gcc`) — 已测试版本 13.3.rel1
- **CMake** 3.13+
- **OpenOCD** — 用于通过 ST-Link 烧录和调试
- **VS Code**（推荐）— 安装 *Cortex-Debug* 和 *CMake Tools* 扩展

### STM32 厂商文件（不在仓库中）

STM32 HAL 和 CMSIS 文件**不在仓库中**（因体积较大已被 gitignore），需要从
[STM32CubeF1](https://github.com/STMicroelectronics/STM32CubeF1) 固件包
或 STM32CubeMX 生成的工程中获取。

在 `bsp/stm32f103rct6-m3-alientek/` 下创建如下目录结构：

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

### OpenOCD 配置

`openocd.cfg` 文件包含 OpenOCD 接口和目标脚本的路径，请根据实际安装位置修改：

```tcl
# 默认路径（Windows 风格）：
source D:/openocd/openocd/scripts/interface/stlink.cfg
source D:/openocd/openocd/scripts/target/stm32f1x.cfg
```

---

## 编译

### 命令行

```bash
cmake -S bsp/stm32f103rct6-m3-alientek \
      -B build/stm32f103rct6-m3-alientek \
      -DCMAKE_TOOLCHAIN_FILE=bsp/stm32f103rct6-m3-alientek/toolchain-arm-none-eabi.cmake \
      -DCMAKE_BUILD_TYPE=Debug

cmake --build build/stm32f103rct6-m3-alientek
```

编译产物（`Firmware.elf`、`.hex`、`.bin`、`.map`）输出到
`bsp/stm32f103rct6-m3-alientek/Outputs/`。

### VS Code

在仓库根目录打开 VS Code。`.vscode/` 目录中预置了任务和启动配置，支持一键编译调试：

- **编译:** `Ctrl+Shift+B` → 选择 CMake 任务
- **调试:** `F5` → 启动 OpenOCD + GDB，在 `main()` 处暂停

---

## 烧录与调试

### OpenOCD 烧录

```bash
openocd -f bsp/stm32f103rct6-m3-alientek/openocd.cfg \
        -c "program bsp/stm32f103rct6-m3-alientek/Outputs/Firmware.elf verify reset exit"
```

### GDB 调试

```bash
# 终端 1 — 启动 OpenOCD
openocd -f bsp/stm32f103rct6-m3-alientek/openocd.cfg

# 终端 2 — 连接 GDB
arm-none-eabi-gdb bsp/stm32f103rct6-m3-alientek/Outputs/Firmware.elf \
    -ex "target extended-remote :3333" \
    -ex "monitor reset init" \
    -ex "continue"
```

### VS Code Cortex-Debug

`.vscode/launch.json` 中预置了调试配置 **"STM32F103 Debug (OpenOCD)"**：
- 启动前自动执行 CMake 编译
- 使用本地 `openocd.cfg` 启动 OpenOCD
- 连接 GDB 并在 `main()` 处断点

---

## 串口终端

以 **115200 波特率、8N1** 连接开发板串口：

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

> **注意：** 部分串口工具在发送时自动追加 `\r\n`，会导致多出一个空 prompt 行。
> 不影响功能，仅为显示问题。

---

## 配置 (`Users/nd_config.h`)

| 宏 | 值 | 说明 |
|----|-----|------|
| `ND_CPU_CLOCK_HZ` | `64000000UL` | CPU 主频（HSI/PLL） |
| `ND_TICKS_PER_SEC` | `1000` | 系统节拍率 |
| `ND_CFG_TICKLESS` | `N` | Tick 模式（周期性 SysTick） |
| `ND_CFG_DEBUG` | `N` | 调试输出关闭 |
| `ND_IDLE_STACK_SIZE` | `512` | 空闲线程栈大小 |
| `ND_NAME_MAX_SIZE` | `16` | 线程名最大长度 |

---

## 工程结构

```
bsp/stm32f103rct6-m3-alientek/
├── CMakeLists.txt                   # 顶层 CMake（MCU 选项、子目录）
├── toolchain-arm-none-eabi.cmake    # 交叉编译器工具链
├── stm32f103rct6.ld                 # 链接脚本（256K Flash, 48K RAM）
├── openocd.cfg                      # OpenOCD 配置（ST-Link + STM32F1x）
├── Outputs/                         # 编译产物（.elf, .hex, .bin）
├── Drivers/
│   ├── CMSIS/                       # CMSIS 头文件 + 启动文件（厂商提供）
│   ├── STM32F1xx_HAL_Driver/        # HAL 库（厂商提供）
│   └── BSP/
│       ├── led/                     # LED 驱动
│       └── usart/                   # USART1 驱动（中断接收、寄存器发送）
└── Users/
    ├── CMakeLists.txt               # 应用编译目标
    ├── main.c                       # 入口、bsp_init、内核启动
    ├── app.c / app.h                # 线程创建
    ├── shell_port.c                 # Shell 后端（UART I/O、环形缓冲）
    ├── nd_config.h                  # RTOS 配置
    └── stm32f1xx_hal_conf.h        # HAL 模块选择
```

应用通过**相对路径**引用仓库根目录下的 NerdRTOS 内核、架构（Cortex-M3）、
组件（shell、ntest）和库（rbtree）源文件。

---

## 内核启动流程

```
main()
├── nd_kernel_lock()            # 全局关中断
├── HAL_Init()                  # 初始化 HAL 时基
├── system_clock_init()         # 72 MHz HSI/PLL
├── usart_init(115200)          # USART1：轮询发送、中断接收
├── nd_hw_tick_init()           # SysTick @ 1000 Hz
├── nd_system_heap_init()       # 从链接符号初始化堆
├── nd_led_init()               # LED GPIO 初始化
├── uart_puts("Hello Nerd RTOS!\r\n")  # 启动信息
├── nd_scheduler_init()         # 空闲线程 + 就绪队列
├── nd_app_init()               # 创建 led_blink + shell 线程
└── nd_scheduler_start()        # 首次上下文切换 → 永不返回
```

---

## 调试技巧

### HardFault 处理器

Cortex-M3 的 HardFault 处理函数位于 `arch/arm/m3/hardfault.c`，
发生异常时会输出所有关键寄存器（R0-R3、R12、LR、PC、PSR、CFSR、
HFSR、MMFAR、BFAR）。通过 GDB 连接即可在调试控制台查看输出。

### 栈使用量监控

`ps` 命令通过扫描线程创建时填充的 0xAA 水印来报告每个线程的栈使用量。
如果使用量接近栈大小，应在 `app.c` 中增大对应线程的栈空间。

### 编译警告检查

```bash
cmake --build build/stm32f103rct6-m3-alientek 2>&1 | tee build.log
python3 .github/scripts/check_build.py build.log
```

### 代码风格检查

```bash
python3 .github/scripts/check_style.py
```
