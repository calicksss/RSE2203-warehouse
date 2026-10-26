"""Checks that every module only #includes modules it is allowed to depend on.

The layering (see docs/ARCHITECTURE.md) keeps the work packages independent:
for example, ds must never include shop, and navigation must never include
picking. CMake would not notice such a mistake (all headers share one include
folder), so CI runs this script.

Usage:  python tools/check_dependencies.py      (exit code 1 if a rule is broken)
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

ALLOWED = {
    "core": {"core"},
    "ds": {"ds"},
    "shop": {"core", "ds", "shop"},
    "navigation": {"core", "ds", "navigation"},
    "picking": {"core", "ds", "navigation", "picking"},
    "fulfilment": {"core", "ds", "shop", "navigation", "picking", "fulfilment"},
    "sim": {"core", "ds", "shop", "navigation", "picking", "fulfilment", "sim"},
    "ui": {"core", "ds", "shop", "navigation", "picking", "fulfilment", "sim", "ui"},
}
INCLUDE = re.compile(r'#include\s+"([a-z]+)/')


def main():
    problems = 0
    for module, allowed in ALLOWED.items():
        folder = os.path.join(ROOT, "modules", module)
        for dirpath, _, files in os.walk(folder):
            if os.path.basename(dirpath) == "tests":
                continue   # tests may include anything they need
            for name in files:
                if not name.endswith((".hpp", ".cpp")):
                    continue
                path = os.path.join(dirpath, name)
                with open(path, encoding="utf-8") as f:
                    for lineno, line in enumerate(f, 1):
                        m = INCLUDE.search(line)
                        if m and m.group(1) in ALLOWED and m.group(1) not in allowed:
                            rel = os.path.relpath(path, ROOT)
                            print(f"{rel}:{lineno}: module '{module}' must not include '{m.group(1)}/...'")
                            problems += 1
    print("dependency check:", "OK" if problems == 0 else f"{problems} problem(s)")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
