# raspi-pico2

## 当前状态
最小系统，仅有串口 printf 功能。

串口引脚：
| 引脚 | 引脚号 |
| ----- | ----- |
| UART0_TX | GP0 |
| UART0_RX | GP1 |
| GND | GND |

## Bulid
首先确保已设置 __PICO_SDK_PATH__
```
mkdir build && cd build && cmake .. -DPICO_BOARD=pico2 -DCMAKE_BUILD_TYPE=Debug
make
```
生成的 uf2 烧录到板子即可。
