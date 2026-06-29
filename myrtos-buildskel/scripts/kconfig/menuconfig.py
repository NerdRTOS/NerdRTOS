#!/usr/bin/env python3
"""
menuconfig.py <Kconfig_root> <dotconfig_path>

启动交互式 menuconfig 界面。保存后只会更新 .config，
autoconf.h 仍然要靠 genconfig.py 在下一次 cmake configure 时重新生成，
所以 menuconfig 之后必须重新跑一遍 cmake（这跟 Zephyr/ESP-IDF 的流程一致）。
"""
import os
import sys

import kconfiglib
from menuconfig import menuconfig  # kconfiglib 自带的 curses 菜单实现


def main() -> int:
    if len(sys.argv) != 3:
        print(f"用法: {sys.argv[0]} <Kconfig根文件> <.config路径>", file=sys.stderr)
        return 1

    kconfig_root, dotconfig_path = sys.argv[1:3]

    kconf = kconfiglib.Kconfig(kconfig_root)

    if os.path.exists(dotconfig_path):
        kconf.load_config(dotconfig_path)

    # menuconfig 模块通过这个环境变量决定保存到哪个文件
    os.environ["KCONFIG_CONFIG"] = dotconfig_path

    menuconfig(kconf)
    return 0


if __name__ == "__main__":
    sys.exit(main())
