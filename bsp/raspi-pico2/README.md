# raspi-pico2

This directory is no longer a standalone CMake entry. It is kept as the Pico2 executable/startup backend while the Zephyr-style board-target build owns board, SoC, sample, and metadata selection.

## Serial Wiring

| Signal | Pin |
| --- | --- |
| UART0_TX | GP0 |
| UART0_RX | GP1 |
| GND | GND |

## Build

Set `PICO_SDK_PATH` first, then configure from the repository root.

M33 domain:

```bash
cmake -S . -B build-pico2-m33 -G Ninja -DBOARD=pico2/rp2350a/m33 -DPICO_SDK_PATH="$PICO_SDK_PATH"
cmake --build build-pico2-m33
```

Hazard3 RISC-V domain:

```bash
cmake -S . -B build-pico2-hazard3 -G Ninja -DBOARD=pico2/rp2350a/hazard3 -DPICO_SDK_PATH="$PICO_SDK_PATH"
cmake --build build-pico2-hazard3
```

Both Pico2 domains through sysbuild:

```bash
cmake -S sysbuild -B build-sys-pico2 -G Ninja -DNERD_DOMAIN_PICO_SDK_PATH="$PICO_SDK_PATH"
cmake --build build-sys-pico2 --target domains
```