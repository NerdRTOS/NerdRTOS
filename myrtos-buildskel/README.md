# MyRTOS 构建骨架（arch / soc / board 三层 Kconfig + CMake + DTS）

这是一个最小骨架，演示 Zephyr 的 board -> soc -> arch 分层思路怎么用纯 CMake + kconfiglib 落地。
当前重点是把“配置怎么流动、文件该放哪、board target 怎么命名、配置片段怎么合并、硬件事实怎么从 DTS 生成头文件、arch/soc/board 构建职责怎么分层”跑出一个可验证闭环。

## 当前 board target

```bash
cmake -S . -B build-pico2-m33      -G Ninja -DCMAKE_C_COMPILER=C:/MinGW/bin/gcc.exe -DBOARD=pico2/rp2350a/m33
cmake -S . -B build-pico2-hazard3  -G Ninja -DCMAKE_C_COMPILER=C:/MinGW/bin/gcc.exe -DBOARD=pico2/rp2350a/hazard3
cmake -S . -B build-qemu-a9        -G Ninja -DCMAKE_C_COMPILER=C:/MinGW/bin/gcc.exe -DBOARD=qemu_a9
cmake -S . -B build-nucleo-f429zi  -G Ninja -DCMAKE_C_COMPILER=C:/MinGW/bin/gcc.exe -DBOARD=nucleo_f429zi
```

`BOARD=pico2` 目前保留为兼容别名，等价于 `BOARD=pico2/rp2350a/m33`。

## 目录结构

```text
.
├── Kconfig                  # 顶层入口：source boards -> soc -> arch
├── CMakeLists.txt           # 顶层构建入口，串起 arch -> soc -> board -> app
├── cmake/kconfig.cmake      # 驱动 DTS + Kconfig，并把 Kconfig 结果接进 CMake 变量
├── scripts/
│   ├── dts/gen_devicetree.py    # 最小 DTS 子集 -> devicetree_generated.h
│   └── kconfig/
│       ├── genconfig.py         # config fragments -> .config/autoconf.h
│       └── menuconfig.py        # 交互式 menuconfig
├── arch/                    # 架构层：startup、架构头文件、架构 linker script、toolchain profile
│   ├── arm_cortex_m/
│   ├── arm_cortex_a/
│   └── riscv/
├── soc/                     # 芯片层：select 架构，提供 soc.h/soc_init() 和 SoC linker fragment
│   ├── raspberrypi/rp2350/
│   ├── qemu/cortex_a9/
│   └── st/stm32f4/
├── boards/                  # 板级层：select 芯片/CPU target，提供 board.h/board_init()、DTS、board linker fragment
│   ├── raspberrypi/pico2/
│   ├── qemu/qemu_a9/
│   └── st/nucleo_f429zi/
└── app/                     # 应用层：只链接 board，不直接关心 arch/soc
    ├── prj.conf
    └── boards/*.conf
```

## 配置流动

```text
BOARD=pico2/rp2350a/m33
  -> boards/raspberrypi/pico2/pico2_rp2350a_m33_defconfig
  -> CONFIG_BOARD_PICO2_RP2350A_M33=y
  -> select BOARD_PICO2
  -> select SOC_RP2350A_M33
  -> select SOC_RP2350A -> SOC_RP2350
  -> select ARCH_ARM_CORTEX_M
```

```text
BOARD=pico2/rp2350a/hazard3
  -> boards/raspberrypi/pico2/pico2_rp2350a_hazard3_defconfig
  -> CONFIG_BOARD_PICO2_RP2350A_HAZARD3=y
  -> select BOARD_PICO2
  -> select SOC_RP2350A_HAZARD3
  -> select SOC_RP2350A -> SOC_RP2350
  -> select ARCH_RISCV
```

用户入口仍然是 BOARD；SoC 和 arch 由 Kconfig 自动推导。

## 配置片段合并顺序

第一次 configure 或配置片段内容变化时，`cmake/kconfig.cmake` 会按这个顺序合并：

```text
1. boards/.../<board-target>_defconfig
2. app/prj.conf
3. app/boards/<normalized-board-target>.conf
```

后面的片段可以覆盖前面的片段。如果 `.config` 已存在且配置片段没有变化，构建系统会继续使用 `.config`，这样可以保留 `menuconfig` 修改过的内容。若配置片段发生变化，会重新从片段生成 `.config`。

## Devicetree 流动

这个骨架实现的是最小 DTS 子集，不是完整 dtc：

```text
boards/.../<board-target>.dts
  -> scripts/dts/gen_devicetree.py
  -> build/include/generated/devicetree_generated.h
```

DTS 负责描述硬件事实：memory、flash、chosen console、chosen timer、uart base/irq/baud、timer base/irq/clock。

C 代码通过生成头使用这些宏：

```c
#include "devicetree_generated.h"

DT_MODEL
DT_SRAM_BASE
DT_SRAM_SIZE
DT_FLASH_BASE
DT_FLASH_SIZE
DT_CONSOLE_BASE
DT_CONSOLE_IRQ
DT_TIMER_CLOCK_FREQUENCY
```

边界保持 Zephyr 风格：

```text
DTS: 硬件事实和启动期硬件配置
Kconfig: 软件功能开关和构建选项
```

## 构建分层

CMake target 依赖关系是：

```text
app -> board -> soc -> arch -> myrtos_kernel
```

每层职责：

```text
arch: startup 占位文件、arch.h、架构 linker script、toolchain profile
soc: soc_init()、soc.h、SoC linker fragment
board: board_init()、board.h、DTS、board linker fragment
app: 应用代码，只链接 board
```

configure 后会生成：

```text
build/build_layers.txt
```

里面记录当前目标实际选中的：

```text
TOOLCHAIN_PROFILE
ARCH_STARTUP_SOURCE
ARCH_LINKER_SCRIPT
SOC_LINKER_SCRIPT
BOARD_LINKER_SCRIPT
```

这一阶段不会把裸机 linker script 强行传给 host linker；真实迁移到 NerdRTOS 主工程时，再把这些变量接到交叉工具链和 `target_link_options()`。

## 怎么跑

```bash
pip install kconfiglib

cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER=C:/MinGW/bin/gcc.exe -DBOARD=pico2/rp2350a/m33
cmake --build build
./build/app/app.exe

cmake --build build --target menuconfig
cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER=C:/MinGW/bin/gcc.exe -DBOARD=pico2/rp2350a/m33
cmake --build build
```

## Sysbuild / Domain

`sysbuild/` 是一个简化版 Zephyr sysbuild，用上层 CMake 管理多个普通 MyRTOS domain build。
每个 domain 都有独立的：

```text
.config
autoconf.h
devicetree_generated.h
build_layers.txt
app/app.exe
```

默认 domain：

```text
pico2_m33      -> BOARD=pico2/rp2350a/m33
pico2_hazard3  -> BOARD=pico2/rp2350a/hazard3
```

运行默认双 domain：

```bash
cmake -S sysbuild -B build-sys -G Ninja -DMYRTOS_DOMAIN_C_COMPILER=C:/MinGW/bin/gcc.exe
cmake --build build-sys
cmake --build build-sys --target pico2_m33_run
cmake --build build-sys --target pico2_hazard3_run
```

打开额外 domain：

```bash
cmake -S sysbuild -B build-sys-all -G Ninja \
  -DMYRTOS_DOMAIN_C_COMPILER=C:/MinGW/bin/gcc.exe \
  -DSB_CONFIG_QEMU_A9_DOMAIN=ON \
  -DSB_CONFIG_NUCLEO_F429ZI_DOMAIN=ON
cmake --build build-sys-all
```

sysbuild 会生成：

```text
build-sys/domains.yaml
build-sys/domains.txt
build-sys/domains/<domain>/...
```

这一步给后续 pico2 多核固件、bootloader + app、多 image 打包、flash/debug 统一入口打基础。

下一步建议开始做“迁移规划”：把 `myrtos-buildskel` 里已经验证过的结构映射到 `NerdRTOS-dev` 主工程目录，先列迁移清单和风险点，再开始落地。