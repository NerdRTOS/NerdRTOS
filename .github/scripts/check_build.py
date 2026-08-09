#!/usr/bin/env python3
"""Parse GCC build output and report warnings."""

import re
import sys

from ci_paths import is_third_party_path

WARNING_RE = re.compile(
    r"^(.+?):(\d+):(\d+): warning: (.+?) \[(-W.+?)\]$"
)

def main():
    if len(sys.argv) < 2:
        print("Usage: check_build.py <build_log>")
        sys.exit(1)

    warnings = []
    ignored_warnings = 0

    with open(sys.argv[1], "r") as f:
        for line in f:
            m = WARNING_RE.match(line.strip())
            if m:
                if is_third_party_path(m.group(1)):
                    ignored_warnings += 1
                    continue
                warnings.append({
                    "file": m.group(1),
                    "line": m.group(2),
                    "msg": m.group(4),
                    "flag": m.group(5),
                })

    if not warnings:
        print("Build clean: no project warnings found.")
        if ignored_warnings:
            print(f"Ignored {ignored_warnings} third-party warning(s).")
        sys.exit(0)

    by_flag = {}
    for w in warnings:
        by_flag.setdefault(w["flag"], []).append(w)

    print(f"Found {len(warnings)} warning(s):\n")
    for flag, items in sorted(by_flag.items()):
        print(f"  {flag}: {len(items)}")
        for w in items:
            print(f"    {w['file']}:{w['line']}: {w['msg']}")

    print(f"\nTotal: {len(warnings)} warning(s)")
    sys.exit(1)


if __name__ == "__main__":
    main()
