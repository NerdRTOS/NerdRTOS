# Porting Guide

## NerdRTOS 架构分层
```
NerdRTOS/
├── arch/                  # Architecture ports
├── bsp/                   # Board support packages
├── kernel/                # Core kernel
├── include/               # Public API headers
├── lib/                   # Utility libraries
├── components/            # Optional components
└── doc/                   # Design documents
```

### arch 层
**arch 层**实现与 CPU 架构绑定的机制：
- 线程栈帧的布局和初始化
- 中断的开关与状态保存/恢复
- 上下文切换的触发与执行

这些机制只取决于 CPU 本身，同一架构在不同具体板子上的实现是一致的。arch 层的设计目标是：任何一个新的 CPU 架构都应该能通过新增一份 arch 实现接入 NerdRTOS，而无需修改 `kernel/` 中的任何代码。

### BSP 层
BSP 只实现与具体板子绑定的内容：
- 启动代码与链接脚本
- 高精度定时器的硬件驱动
- 编译期配置（`nd_config.h`）：CPU 主频、tick 频率、空闲栈大小等
- 板上外设驱动（用于 shell 的 uart 等）

BSP 层并不要求所有底层代码都从零编写。**只要厂商提供了 SDK 或 HAL，BSP 实现就可以直接调用它们**，把节省下来的时间留给真正与 NerdRTOS 集成相关的部分（启动流程、时钟源接入、向量表对接）。

`bsp/raspi-pico2/` 就是一个完全建立在 Pico SDK 之上的 BSP：

`CMakeLists.txt` 通过 `pico_sdk_import.cmake` 引入整个 SDK，由 SDK 负责启动文件、链接脚本、Flash 启动头、向量表等基础设施

作为对比，`bsp/MPS2-AN505/` 因为**面向裸金属、没有现成 SDK 可用**，所以 `startup.c` 和 `link.ld` 需要自己写。两种风格的 BSP 在教程里都会作为案例出现，可以根据手上的板子有没有可用 SDK，自行选择参考哪一份。

用 SDK 不等于放弃分层纪律。SDK 调用只能出现在 `bsp/` 目录内部，`kernel/` 和 `arch/` 不依赖任何 SDK 来实现。

---

## arch 移植
### 中断开关原语
中断开关有三个接口：
```c
#define nd_kernel_def()     nd_size_t status
#define nd_kernel_lock()    status = nd_hw_irq_save()
#define nd_kernel_unlock()  nd_hw_irq_restore(status)
```

内核存在大量**共享数据结构**，可能同时被中断上下文和线程上下文保护，如果两个执行流对一份数据进行了读写交叠，会破坏**数据结构的一致性**。因此我们需要关中断，开中断来保护。关中断使 CPU 在临界区内不会切换到任何其他执行上下文，从而把临界区内的一组操作变成对外不可分割的整体。NerdRTOS 把这个动作抽象成两个接口，由 arch 层实现：
```c
nd_size_t nd_hw_irq_save(void);
void      nd_hw_irq_restore(nd_size_t level);
```

实现 `nd_hw_irq_save`，需要按顺序完成三件事：
- 读取当前 CPU 的中断使能状态
- 关闭中断
- 将读取到的状态作为返回值交给调用方

实现 `nd_hw_irq_restore` 只需要一步：
- 把调用方传入的 `level` 应用回硬件，让中断使能状态精确恢复到 `nd_hw_irq_save` 被调用前的样子。

#### 设计约束
- **原子性**：`save` **读状态**到**关中断**不可以有可能被中断打断的窗口。因此 `save` **不能用 C 实现**（两条独立语句之间可能被中断打断），而要借助下列之一：
  - **汇编**：用一条**读-改合一**的指令同时完成读和关。如果目标架构没有这种合一指令，就用两条相邻的汇编指令并依赖它们之间不会有中断改变中断屏蔽位这一架构不变量。
  - **编译器内置原语**：要求该原语对**原子读-改**有明确保证。
  - **关闭编译器优化的内联汇编块**：保证"读"和"关"不会被调度到不该出现的位置。
- `level` 是不透明值，由 arch 层自行决定格式。内核拿到 `save` 的返回值后只做一件事：在 `restore` 时原样传回。内核不检查、比较、修改 `level` 的值。这意味着每个架构可以选择最适合自己的表示方式：
  - 如架构天然拥**有**总中断屏蔽寄存器，直接把整个寄存器值作为 `level` 最简单。如 ARM Cortex-M 的 `PRIMASK` 就是一个专门用来屏蔽中断的寄存器。
  - 如架构**没有**总中断屏蔽寄存器，中断使能在一个更大的状态寄存器里，这时候不能**只保存中断使能的第 N 位**，否则 `restore` 会漏掉其他同样影响中断是否被响应的位。arch 需要**保存和恢复**整个寄存器。
  - 如果架构（例如支持多条中断线的）有多组使能位需要同时保存，`level` 可以是它们的组合。
  - 如果架构有多级中断优先级屏蔽，`level` 应当是**屏蔽阈值**而非简单的开/关。

### 栈初始化
#### 为什么需要栈初始化
上下文的保存和恢复是通用的，只会按照一套流程从栈上读值到 CPU 寄存器。对于被切回来的线程，栈上的内容是自己上次切出去的时候保存的，而对于从未运行过的线程，栈上的字节取决于内存的来源（释放的残留、BSS 清零、随机值），无法保证在恢复上下文中读取的是合法值。因为**上下文恢复过程不验证栈上值的语义合法性**，栈上是什么值就加载什么值，故障会在后续指令执行时才暴露，关键字段如果非法会导致处理器异常（如 `HardFault`、`UsageFault`）等错误。

栈初始化的作用，就是在线程创建的时候，手动构造一个上下文内容，这个上下文内容将被作为每个线程第一次执行的初始值。

#### 栈初始化的实现
NerdRTOS 中栈初始化通过 `nd_hw_stack_init` 来完成：
```c
void *nd_hw_stack_init(void       *entk_fun,
                       void       *parameter,
                       nd_uint8_t *stack_addr,
                       void       *exit_fun);
```
- 入参：
  - entk_fun：线程入口函数地址
  - parameter：入口函数的首参数
  - stack_addr：栈区的高地址边界（栈向低地址生长，故为栈顶初值）
  - exit_fun：线程返回时的跳转目标
- 返回值：初始化完成后的栈指针

##### 设计上下文帧结构
上下文帧描述的是线程上下文在栈中的内存布局。

`nd_hw_stack_init()` 会按该布局构造线程的初始上下文，上下文保存与恢复虽然通常由汇编通过压栈、弹栈指令完成，并不会直接操作这个 C `struct`，但它们在栈上形成和消费的字段集合、字段顺序与字段 `offset` 必须与该结构描述的布局一致。

**设计输入来自两部分**：
- **架构约束**：
  - **ABI** 对寄存器的分类：`caller-saved`、`callee-saved`、参数寄存器、返回地址寄存器（`LR`）、程序计数器（`PC`）。
  - **异常进入/返回硬性约束**：移植架构的异常进入是否有硬件自动栈操作。如果有，这部分字段与字段顺序有硬件规范固定，软件帧对齐。如果没有，全部寄存器的保存由软件来完成。
  - 栈对齐要求
- **切换方案**：
  - 架构支持的切换来源：协作切换、异步中断抢占、同步陷入等
  - 上下文切换是通过普通控制流完成，还是借助异常返回完成
  - 是否需要多种帧结构以及对应的机制

**字段集合**：
- **协作切换**：协作式切换通常发生在线程主动调用 `yield` 这类函数时。按照 ABI 约定，调用者不能假设 `caller-saved` 寄存器在函数调用后仍保持原值。如果其中的值后续还要用，调用者应当在调用前自行保存。`caller-saved` 寄存器可丢弃，只需保存 `callee-saved` 寄存器，以及栈指针、返回地址和状态寄存器等必要控制信息。
- **抢占切换**：线程有可能在任意执行点被打断，全部通用寄存器的值都可能是**正在使用中**的，必须全部保存。

按寄存器类别判断是否必须纳入帧：
| 类别 | 是否纳入 | 判断依据 |
|------|----------|----------|
| 程序计数器 / 异常返回地址 | 必须 | 决定线程从何处继续执行 |
| 处理器状态寄存器 | 必须 | 运行模式、指令集状态、中断屏蔽等 |
| 首参数寄存器 | 首帧必须 | 入口函数的 `parameter` 由此传入 |
| 返回地址寄存器 | 首帧必须（若允许入口函数返回） | 定义入口返回时的跳转目标 |
| Callee-saved 通用寄存器 | 必须 | 切走再切回后，这些寄存器值必须保持不变 |
| Caller-saved 通用寄存器 | 取决于切换边界 | 仅含协作切换边界时可省略；含抢占切换边界时须纳入 |
| FPU / 协处理器寄存器 | 架构相关 | 架构支持浮点或扩展状态的需按其硬件规则设计 |

**确定字段排列顺序**：
1. **硬件强制**：如移植架构异常进入时硬件会自动压栈部分寄存器，这些寄存器的内容和顺序由硬件决定，结构体的定义与其对应。
2. **软件保存**：后续计划使用的 `load`/`store` 指令族安排顺序。
3. **结构体与栈布局对齐**：结构体字段顺序必须与栈上从低到高地址的实际布局一致。

##### 字段初始值
首次切入时，上下文恢复路径会把这些初始值加载进寄存器，CPU 需要能正确开始执行 `entk_fun(parameter)`，从入口函数 `return` 的时候，必须能跳转到 `exit_fun` 处。

赋值要求分为三类：
- **ABI 契约字段**：
**必须赋值**
恢复路径加载后直接给 C 代码使用，该值决定入口函数能否正确启动：

| 寄存器 | 启动值 | 作用 |
|--------|--------|------|
| 首参数寄存器 | `parameter` | 作为 `entk_fun` 的第一个参数 |
| 程序计数器 / 异常返回地址 | `entk_fun` | 恢复后首条执行的指令地址 |
| 返回地址寄存器（`LR` 一类） | `exit_fun` | 入口函数 `return` 时的跳转目标 |

- **架构控制字段**：
**必须赋值**
架构控制字段决定恢复完成后 CPU 处于何种运行状态。若未赋值或值非法，即使 `PC` 正确，CPU 进入的仍可能是错误的运行模式、错误的指令集状态或错误的中断屏蔽配置。

- **占位字段**：
**不强制要求**
占位字段包括 `callee-saved`、`caller-saved` 通用寄存器等。这些寄存器首次运行时 `entk_fun` 会重新写入，其初值不影响正确性。事实上 NerdRTOS 在线程创建时用 `0xAA` 填充整个栈，让栈字节构成确定但仍然非法的值（用于栈水位统计和调试时辨识未写入区域）。

##### 帧的构造
帧的构造分为三步，以 ARM Cortex-M33 为例：
- **定位**：栈向低地址增长，帧位于栈顶，帧的最低地址处就是最终返回给调度器的 `sp` 值。

```c
stk  = stack_addr + sizeof(nd_uint32_t);
stk  = (nd_uint8_t *)ND_ALIGN_DOWN((nd_uint32_t)stk, STACK_ALIGN);
stk -= sizeof(struct context_frame);
frame = (struct context_frame *)stk;
```

`STACK_ALIGN` 取移植架构的 ABI 规定的栈对齐值。

- **字段初始化**：
  - **占位字段**：随意，清零或者保持原始值。
  - **ABI 契约字段**：依照 `entk_fun`、`parameter`、`exit_fun` 填入对应寄存器槽。
  - **架构控制字段**：写入[启动常量值](#derive-constants)。

- **返回栈指针**：
函数返回栈底地址。

[参考实现](../arch/arm/m33/port.c)（ARM Cortex-M33）

### 上下文切换
上下文切换通过以下两个接口实现：
```c
void nd_hw_do_switch_first(void);
void nd_hw_do_switch(void);
```
- `nd_hw_do_switch_first`：用于首次切换，也就是 `nd_current_thread == NULL`，没有上下文可保存。
- `nd_hw_do_switch`：用于正常的切换（非首次）。

上下文切换有两种类型，**协作式和抢占式**：
- 协作式上下文切换指的是一个线程将主动控制权给另外一个线程，通常有两种情况：
  - 线程通过 `yield` 显式让出
  - 当一个线程试图获取一个不可用的资源，并且愿意等到资源可用的时候
- 如果当前运行的线程是可抢占的，当 ISR 或线程某个动作让优先级高于当前运行线程被调度的时候，就会发生抢占式上下文切换。

无论哪种时机触发，执行点最终都收束到一段代码——换栈，恢复 `callee-saved`，跳转到新 `PC`，需要保证**同一时刻只有一个切换在执行**。

### 时间基准
NerdRTOS 提供了两种时间基准，分别用于传统的节拍模式和高精度定时器。归属 arch 或 BSP 取决于架构是否带相应硬件。

- 架构自带：arch 实现
- 不自带：BSP 实现

[tick 时基](#tick-时基)

---

## BSP 移植
### nd_config.h
| 宏 | 含义 | 典型值 |
|----|------|--------|
| `ND_CPU_CLOCK_HZ` | CPU 主频（Hz），tick / hrtimer 用它推算 reload | 视板子 |
| `ND_TICKS_PER_SEC` | tick 频率（Hz） | 100 / 1000 |
| `ND_TICKLESS_FREQ` | hrtimer 时基频率（Hz） | 1000000 |
| `ND_CFG_TICKLESS` | 1 启用 tickless 模式，0 走 tick 模式 | 1 |
| `ND_IDLE_STACK_SIZE` | idle 线程栈字节数 | 512 |
| `ND_NAME_MAX_SIZE` | 线程名最大字节数 | 16 |

[参考实现](../bsp/raspi-pico2/nd_config.h)

### 启动代码与链接脚本
BSP 负责把 CPU 从复位向量带到 `main`，路径上有这些事必须完成：`.data` 从 Flash 拷到 RAM、`.bss` 清零、主栈指针初始化、向量表就位。

**SDK 型 BSP**：启动代码和默认链接脚本由 SDK 提供，BSP 只需通过 SDK 的 CMake 钩子挂入想覆盖的部分（如自定义链接脚本）。以 [bsp/raspi-pico2/CMakeLists.txt](../bsp/raspi-pico2/CMakeLists.txt) 为例，`pico_sdk_import.cmake` 引入 Pico SDK，`pico_set_linker_script(nerd nerd.ld)` 注入本 BSP 的链接脚本。

**裸金属 BSP**：自己写 `startup.c` 与 `link.ld`。
- `startup.c`：定义向量表、`Reset_Handler`（依序做 `.data` 拷贝、`.bss` 清零、跳 `main`）、其他 IRQ 占位
- `link.ld`：定义 FLASH / RAM 段与 `.text` / `.data` / `.bss` 布局，导出给 `Reset_Handler` 使用的符号（`__etext`、`__data_start__`、`__bss_start__`、`__StackTop` 等）

### 系统启动
`main` 按以下顺序调用内核与 BSP 自身的初始化：

```c
int main(void)
{
    bsp_init();            /* 板级硬件：UART、时钟源、tick/hrtimer 等 */
    nd_scheduler_init();   /* 初始化就绪队列、idle 线程 */
    nd_app_init();         /* 创建应用线程 */
    nd_scheduler_start();  /* 从首个线程开始运行，不再返回 */
}
```

`bsp_init()` 至少要完成时间基准的初始化——根据 `ND_CFG_TICKLESS` 选择 `nd_hw_hrtimer_init()` 或 `nd_hw_tick_init()`。`nd_app_init()` 是本 BSP 的命名约定，用户也可把线程创建直接写在 `main` 里。

### 时间基准
若目标架构不自带 tick / hrtimer 硬件，对应接口放在 BSP 层实现，规范与 arch 层一致，见 [tick 时基](#tick-时基) / [hrtimer 时基](#hrtimer-时基)。

[参考实现](../bsp/raspi-pico2/nd_hrtimer.c)

### shell 适配
NerdRTOS 内核本身不依赖任何外设。启用 shell 组件时，BSP 需提供以下五个符号，把 shell 的收发绑到板上 UART：

```c
void shell_init(void);
char shell_getc(void);
int  shell_putc(char c);
void shell_puts(const char *str);
void shell_printf(const char *fmt, ...);
```

- `shell_init`：一次性初始化 UART 与接收侧同步原语（通常是接收中断 + 信号量）
- `shell_getc`：无字符时阻塞等待
- `shell_putc`：发送一个字符
- `shell_puts` / `shell_printf`：基于前两者构建，需保证多线程调用时不交错输出（参考实现用 `nd_mutex_t`）

[参考实现](../bsp/raspi-pico2/shell_port.c)

---

## 时间基准实现
### tick 时基
tick 是由硬件周期性产生的中断，以固定的频率产生周期性的时钟节拍。

tick 有两个接口：
```c
void        nd_hw_tick_init(void);
nd_uint64_t nd_hw_tick_get_current(void);
```

#### 硬件选择
架构规范定义的周期定时器放 arch 层（如 ARMv8-M 的 SysTick、Cortex-A MPCore 的 private timer），SoC 或板级引入的定时器放 BSP 层。

#### 实现
**`nd_hw_tick_init`**：
1. 关闭定时器，避免后续寄存器写入期间发生意外计数
2. 写入 reload 值 `ND_CPU_CLOCK_HZ / ND_TICKS_PER_SEC - 1`
3. 选择时钟源、使能自动重载
4. 使能对应中断
5. 启动计数

`ND_CPU_CLOCK_HZ` 与 `ND_TICKS_PER_SEC` 均为编译期常量，由 BSP 的 `nd_config.h` 提供。arch 层只消费宏，不内嵌具体频率。

**`nd_hw_tick_get_current`**：
返回一个自 tick 启动以来单调递增的计数值。实现上就是一个模块内 `static volatile nd_uint64_t` 由 ISR 每次加一。

#### tick ISR 的本体工作
不论目标架构采用哪种 IRQ 入口协议，tick ISR 本身只做三件事：

1. 清硬件中断 pending（否则 ISR 退出后立刻重新进入）
2. tick 计数加一
3. 调用 `nd_timer_process()`

`nd_timer_process()` 是内核暴露给 arch 的工具函数。它遍历内核定时器链表，执行到期回调，服务三类到期事件：
- 软定时器到期回调
- 线程 `delay` 结束的唤醒
- 时间片到期的轮转标志

[参考实现](../arch/arm/m33/systick.c)（ARM Cortex-M33 SysTick）

### hrtimer 时基
hrtimer 是按内核请求单次触发的定时器，用于 tickless 模式，时间分辨率远高于 tick。

hrtimer 有五个接口：
```c
void        nd_hw_hrtimer_init(void);
nd_uint64_t nd_hw_hrtimer_get_current(void);
void        nd_hw_hrtimer_set_expire(nd_uint64_t expire);
void        nd_hw_hrtimer_trigger(void);
void        nd_hw_hrtimer_trigger_clear(void);
```

#### 硬件选择
hrtimer 需要**单调时基**和**可编程单次告警**两件能力。归属规则与 tick 一致：架构规范自带放 arch，SoC 引入放 BSP。

#### 实现
**`nd_hw_hrtimer_init`**：
1. 启动时基计数器
2. 把告警配置到单次触发模式，关掉自动重载位
3. 使能告警中断，但不编程具体到期时间

**`nd_hw_hrtimer_get_current`**：
返回自启动以来单调递增、单位为 `1/ND_TICKLESS_FREQ` 秒的计数值。把硬件计数器按比例换算即可：
```c
return hw_counter_read() / (ND_CPU_CLOCK_HZ / ND_TICKLESS_FREQ);
```
若硬件计数单位本身就是 `1/ND_TICKLESS_FREQ` 秒，直接返回。

**`nd_hw_hrtimer_set_expire`**：
`expire` 是以 `ND_TICKLESS_FREQ` 为单位的绝对时刻。硬件告警寄存器接受绝对值则直接写入；只接受相对值则先算 `delta = expire - get_current()` 再乘回 CPU 周期数。若 `expire <= get_current()`，写入最小正值让告警立刻触发，避免写 0 或让减法溢出。

**`nd_hw_hrtimer_trigger`** / **`nd_hw_hrtimer_trigger_clear`**：
- `trigger`：强制立即产生一次告警中断，用于内核在 ISR 中唤醒了更紧迫的定时器、需要立刻重算下一次到期时
- `trigger_clear`：取消已编程但尚未到期的告警，用于重新规划到期时间前撤销旧告警

#### hrtimer ISR
hrtimer ISR 本身只做两件事：

1. 清硬件中断 pending
2. 调用 `nd_timer_process()`

hrtimer 无需自增计数，下一次到期时间由内核在每次 ISR 后重新编程。

[参考实现](../bsp/raspi-pico2/nd_hrtimer.c)（RP2350，SoC 引入）

<a id="derive-constants"></a>
> [!TIP]
> **如何从架构手册中推出启动常量值？**
>
> 查架构手册该寄存器的 **bit field** 定义，需要注意的有：
> - 运行模式位：RTOS 跑在什么特权？（NerdRTOS 目前均跑在特权态，没有用户态）
> - 中断屏蔽位：线程运行的时候是否允许被中断？（NerdRTOS 是抢占式 RTOS，允许）
> - 条件标志位：入口函数对启动标志有没有依赖？（无）
>
> 按字段定义表中每位的位置，将选定的值填入，其余位 0，得到十六进制常量。
