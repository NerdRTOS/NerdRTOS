"""Shared source-path classification for CI checks."""

import re


THIRD_PARTY_PATH_RE = re.compile(
    r"(?:^|/)components/lwip/src(?:/|$)|"
    r"(?:^|/)bsp/[^/]+/Drivers/CMSIS(?:/|$)|"
    r"(?:^|/)bsp/[^/]+/Drivers/[^/]+_HAL_Driver(?:/|$)|"
    r"(?:^|/)bsp/[^/]+/Drivers/BSP/Components(?:/|$)"
)


def is_third_party_path(path):
    """Return whether path belongs to imported, externally maintained code."""
    return THIRD_PARTY_PATH_RE.search(path.replace("\\", "/")) is not None
