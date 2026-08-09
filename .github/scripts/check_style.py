#!/usr/bin/env python3
"""Check source style: trailing whitespace, line endings, tabs, non-ASCII, banned APIs."""

import os
import re
import sys

from ci_paths import is_third_party_path

SCAN_EXT = (".c", ".h")

EXCLUDE_DIR_PARTS = ("build", "pico-sdk", ".git")
EXCLUDE_DIR_PREFIXES = ("arm-gnu-toolchain-",)

EXCLUDE_FILES = (
    "include/lib/rbtree.h",
    "lib/rbtree.c",
)

BANNED_APIS = (
    "printf", "malloc", "free", "calloc", "realloc",
    "sprintf", "strcpy", "strcat",
)
BANNED_SCOPE = ("kernel/", "arch/")
BANNED_RE = re.compile(r"\b(" + "|".join(BANNED_APIS) + r")\s*\(")
NON_ASCII_RE = re.compile(r"[^\x00-\x7f]")

BAD_KEYWORD_RE = re.compile(r"\b(if|while|for|switch)\(")
NO_SPACE_BRACE_RE = re.compile(r"\)\{")

STRING_RE       = re.compile(r'"(?:[^"\\]|\\.)*"')
CHAR_RE         = re.compile(r"'(?:[^'\\]|\\.)*'")
CCOMMENT_RE     = re.compile(r"/\*.*?\*/")
LINECOMMENT_RE  = re.compile(r"//.*$")


def strip_strings_and_comments(line):
    line = STRING_RE.sub('""', line)
    line = CHAR_RE.sub("''", line)
    line = CCOMMENT_RE.sub("", line)
    line = LINECOMMENT_RE.sub("", line)
    return line


def is_excluded_dir(path):
    name = os.path.basename(path)
    if name in EXCLUDE_DIR_PARTS:
        return True
    if any(name.startswith(p) for p in EXCLUDE_DIR_PREFIXES):
        return True
    return is_third_party_path(path)


def iter_files(root):
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [
            d for d in dirnames
            if not is_excluded_dir(os.path.join(dirpath, d))
        ]
        for fn in filenames:
            if not fn.endswith(SCAN_EXT):
                continue
            path = os.path.join(dirpath, fn)
            rel = os.path.relpath(path, root).replace(os.sep, "/")
            if rel in EXCLUDE_FILES:
                continue
            yield rel, path


def check_file(rel, path):
    errors = []
    with open(path, "rb") as f:
        data = f.read()

    if b"\r\n" in data:
        errors.append((rel, 0, "CRLF line endings"))
    if data and not data.endswith(b"\n"):
        errors.append((rel, 0, "file does not end with newline"))

    try:
        text = data.decode("utf-8")
    except UnicodeDecodeError as e:
        errors.append((rel, 0, f"not valid UTF-8: {e}"))
        return errors

    check_banned = any(rel.startswith(s) for s in BANNED_SCOPE)

    lines = text.splitlines()
    blank_run = 0

    for i, line in enumerate(lines):
        idx = i + 1
        if "\t" in line:
            errors.append((rel, idx, "tab character (use 4 spaces)"))
        if line != line.rstrip():
            errors.append((rel, idx, "trailing whitespace"))
        m = NON_ASCII_RE.search(line)
        if m:
            errors.append((rel, idx, f"non-ASCII char {m.group(0)!r} at col {m.start() + 1}"))

        code = strip_strings_and_comments(line)

        m = BAD_KEYWORD_RE.search(code)
        if m:
            errors.append((rel, idx, f"missing space after '{m.group(1)}' keyword"))
        if NO_SPACE_BRACE_RE.search(code):
            errors.append((rel, idx, "missing space between ')' and '{'"))

        if check_banned:
            m = BANNED_RE.search(code)
            if m:
                scope = rel.split("/", 1)[0]
                errors.append((rel, idx, f"banned API '{m.group(1)}' in {scope}/"))

        if line == "":
            blank_run += 1
            if blank_run == 2:
                errors.append((rel, idx, "multiple consecutive blank lines"))
        else:
            blank_run = 0

        if line == "}" and i + 1 < len(lines):
            nxt = lines[i + 1]
            if nxt != "" and not nxt.lstrip().startswith("#"):
                errors.append((rel, i + 2, "missing blank line after top-level '}'"))

    return errors


def main():
    root = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else ".")

    all_errors = []
    for rel, path in iter_files(root):
        all_errors.extend(check_file(rel, path))

    if not all_errors:
        print("Style check clean.")
        sys.exit(0)

    by_file = {}
    for rel, line, msg in all_errors:
        by_file.setdefault(rel, []).append((line, msg))

    print(f"Found {len(all_errors)} style issue(s):\n")
    for rel in sorted(by_file):
        print(f"  {rel}:")
        for line, msg in by_file[rel]:
            if line:
                print(f"    :{line}: {msg}")
                print(f"::error file={rel},line={line}::{msg}")
            else:
                print(f"    {msg}")
                print(f"::error file={rel}::{msg}")

    print(f"\nTotal: {len(all_errors)} issue(s)")
    sys.exit(1)


if __name__ == "__main__":
    main()
