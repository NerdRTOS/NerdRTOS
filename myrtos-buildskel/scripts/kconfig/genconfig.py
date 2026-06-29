#!/usr/bin/env python3
"""
genconfig.py <Kconfig_root> <dotconfig_path> <autoconf_h_path> [config fragments...]

构建系统配置阶段真正驱动 Kconfig 的脚本，对应 Zephyr/ESP-IDF 里的：
"merge config fragments -> 校验依赖 -> 落地 .config -> 生成 autoconf.h"。

输入片段按命令行顺序合并，后面的片段可以覆盖前面的片段。常见顺序是：

1. board defconfig
2. app/prj.conf
3. app/boards/<board>.conf

如果没有传入片段但 dotconfig_path 已存在，则加载已有 .config，用于保留 menuconfig
之后的用户配置。
"""
import os
import sys

import kconfiglib


def main() -> int:
    if len(sys.argv) < 4:
        print(
            f"用法: {sys.argv[0]} <Kconfig根文件> <.config路径> <autoconf.h路径> [配置片段...]",
            file=sys.stderr,
        )
        return 1

    kconfig_root, dotconfig_path, autoconf_h_path = sys.argv[1:4]
    config_fragments = sys.argv[4:]

    kconf = kconfiglib.Kconfig(kconfig_root)
    # 后面的 fragment 覆盖前面的 fragment 是这个骨架的明确语义，
    # 对应 Zephyr 的 board defconfig -> prj.conf -> board.conf 合并方式。
    kconf.warn_assign_override = False
    kconf.warn_assign_redun = False

    if config_fragments:
        for index, fragment in enumerate(config_fragments):
            replace = index == 0
            print(kconf.load_config(fragment, replace=replace))
    elif os.path.exists(dotconfig_path):
        print(kconf.load_config(dotconfig_path))

    if kconf.warnings:
        for warning in kconf.warnings:
            print(warning, file=sys.stderr)
        return 1

    os.makedirs(os.path.dirname(autoconf_h_path), exist_ok=True)

    kconf.write_config(dotconfig_path)
    kconf.write_autoconf(autoconf_h_path)

    print(f"[kconfig] {dotconfig_path}")
    print(f"[kconfig] {autoconf_h_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())