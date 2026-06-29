<p align="center">
  <h1 align="center">NerdRTOS</h1>
  <p align="center">A lightweight, priority-based preemptive Real-Time Operating System.</p>
</p>

<p align="center">
  <a href="https://github.com/NerdRTOS/NerdRTOS/actions/workflows/build.yml"><img src="https://github.com/NerdRTOS/NerdRTOS/actions/workflows/build.yml/badge.svg" alt="Build"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-MIT-blue.svg" alt="License: MIT"></a>
</p>

---

## Introduction

The NerdRTOS is based on a small-footprint kernel designed for use on
resource-constrained and embedded systems: from simple sensor nodes and
LED wearables to sophisticated embedded controllers and IoT wireless
applications.

The NerdRTOS kernel supports multiple architectures and boards.
The full list of supported targets can be found in the
[`arch/`](arch/) and [`bsp/`](bsp/) directories.


## Building Pico2

Pico2 uses explicit Zephyr-style board targets. The retired `BUILD_CONFIG=pico2` and standalone `bsp/raspi-pico2` CMake entry should not be used for new builds.

Set `PICO_SDK_PATH` first, then build one domain explicitly:

```bash
cmake -S . -B build-pico2-m33 -G Ninja -DBOARD=pico2/rp2350a/m33 -DPICO_SDK_PATH="$PICO_SDK_PATH"
cmake --build build-pico2-m33

cmake -S . -B build-pico2-hazard3 -G Ninja -DBOARD=pico2/rp2350a/hazard3 -DPICO_SDK_PATH="$PICO_SDK_PATH"
cmake --build build-pico2-hazard3
```

To build both Pico2 domains through sysbuild:

```bash
cmake -S sysbuild -B build-sys-pico2 -G Ninja -DNERD_DOMAIN_PICO_SDK_PATH="$PICO_SDK_PATH"
cmake --build build-sys-pico2 --target domains
```
## Licensing

NerdRTOS is released under the [MIT License](LICENSE)
(as found in the `LICENSE` file in the project's
[GitHub repo](https://github.com/NerdRTOS/NerdRTOS)). All source files
in this repository are MIT licensed unless otherwise noted.

## Distinguishing Features

NerdRTOS offers a growing set of kernel features including:

**Extensive Suite of Kernel Services**

 NerdRTOS provides a familiar set of services for embedded development:

 - *Multi-threading Services* for priority-based preemptive and cooperative
   threads with optional time-slice round-robin.
 - *Inter-thread Synchronization Services* for binary semaphores, counting
   semaphores, and mutexes with priority inheritance.
 - *Inter-thread Data Passing Services* for fixed-size message queues and
   event groups.

**Multiple Scheduling Algorithms**

 NerdRTOS provides a focused set of scheduling strategies suited for
 embedded systems:

 - *Preemptive Scheduling* for immediate preemption of lower-priority
   threads by higher-priority ones. Preempted threads are re-queued at
   the head of their priority queue for prompt resumption.
 - *Round-Robin Time Slicing* for fair CPU sharing among equal-priority
   threads via configurable per-thread time slices, with remaining slice
   time tracked across context switches.
 - *Cooperative Yield* allows threads to voluntarily relinquish the CPU
   via `nd_thread_yield()`, triggering a round-robin rotation among peers.
 - *O(1) Priority Lookup* via a 32-bit ready bitmap and hardware CLZ/CTZ
   instructions, with a portable lookup-table fallback for toolchains
   without intrinsic support.

**Timer Management**

 One-shot and periodic software timers managed in a Red-Black tree for
 O(log n) insertion and expiry.

**Tick and Tickless Mode**

 Supports both traditional tick-based timing and tickless mode. In tickless
 mode, the system dynamically adjusts the next wake-up to the nearest
 pending event, eliminating unnecessary periodic interrupts and reducing
 power consumption.

**Cross Architecture**

 The hardware abstraction layer is designed to support multiple CPU
 architectures and boards. See [`arch/`](arch/) and [`bsp/`](bsp/) for
 currently supported targets.

**Shell and Testability**

 Built-in interactive shell for runtime inspection and command execution.
 Includes NTest, a lightweight in-tree test framework with direct shell
 integration.

## Project Structure

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
