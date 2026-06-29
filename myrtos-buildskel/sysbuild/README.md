# MyRTOS sysbuild

This is a small CMake superbuild that models Zephyr sysbuild/domain behavior.
Each domain is a normal MyRTOS build with its own build directory, `.config`, generated headers, and layer metadata.

Default domains:

```text
pico2_m33      -> BOARD=pico2/rp2350a/m33
pico2_hazard3  -> BOARD=pico2/rp2350a/hazard3
```

Example:

```bash
cmake -S sysbuild -B build-sys -G Ninja -DMYRTOS_DOMAIN_C_COMPILER=C:/MinGW/bin/gcc.exe
cmake --build build-sys
cmake --build build-sys --target pico2_m33_run
cmake --build build-sys --target pico2_hazard3_run
```

Optional extra domains:

```bash
cmake -S sysbuild -B build-sys -G Ninja \
  -DMYRTOS_DOMAIN_C_COMPILER=C:/MinGW/bin/gcc.exe \
  -DSB_CONFIG_QEMU_A9_DOMAIN=ON \
  -DSB_CONFIG_NUCLEO_F429ZI_DOMAIN=ON
```

Generated metadata:

```text
build-sys/domains.yaml
build-sys/domains.txt
build-sys/domains/<domain>/build_layers.txt
```