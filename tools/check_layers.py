#!/usr/bin/env python3
"""Check that the core modules form a single downward-directed dependency DAG.

Each module is the set of files sharing a basename (`candy_x.c` + `candy_x.h`).
An edge A -> B means some file of A includes some file of B.  A module may only
depend on strictly lower layers (LAYERS below), so no cycle can appear.
"""
import re
import sys
import glob
from collections import defaultdict

LAYERS = [
    ["candy_log", "candy_reader", "candy_types"],
    ["candy_priv"],
    ["candy_exception", "candy_lib", "candy_object", "candy_token"],
    ["candy_binary", "candy_memory"],
    ["candy_hashmap", "candy_vector"],
    ["candy_buffer", "candy_gc"],
    ["candy_closure", "candy_userdef", "candy_wrap"],
    ["candy_array", "candy_table", "candy_upvalue"],
    ["candy_lexer", "candy_proto"],
    ["candy_parser", "candy_vm"],
    ["candy_state"],
    ["candy", "candy_utils"],
]

ROOTS = ("src",)  # applications (shell/) sit above the library and are not layered
INCLUDE = re.compile(r'^\s*#\s*include\s*"([^"]+)"', re.M)


def module_of(path: str) -> str:
    return path.split("/")[-1].split(".")[0]


def scan():
    files = []
    for root in ROOTS:
        for ext in ("c", "h"):
            files += glob.glob(f"{root}/**/*.{ext}", recursive=True)
    by_suffix = defaultdict(list)
    for path in files:
        parts = path.split("/")
        for i in range(len(parts)):
            by_suffix["/".join(parts[i:])].append(path)
    edges = defaultdict(set)
    for path in files:
        for name in INCLUDE.findall(open(path).read()):
            for target in by_suffix.get(name, []):
                if module_of(target) != module_of(path):
                    edges[module_of(path)].add(module_of(target))
    return sorted(files), edges


def main() -> int:
    files, edges = scan()
    level = {mod: index for index, mods in enumerate(LAYERS) for mod in mods}
    problems = []

    for mod in sorted(edges):
        if mod not in level:
            problems.append(f"{mod}: missing from LAYERS")
            continue
        for dep in sorted(edges[mod]):
            if dep not in level:
                problems.append(f"{mod} -> {dep}: {dep} missing from LAYERS")
            elif level[dep] >= level[mod]:
                problems.append(f"{mod} (L{level[mod]}) -> {dep} (L{level[dep]}): must depend downwards only")

    if problems:
        print("layer check FAILED", file=sys.stderr)
        for problem in problems:
            print("  " + problem, file=sys.stderr)
        return 1
    print(f"layer check OK: {len(files)} files, {len(edges)} modules, {len(LAYERS)} layers")
    return 0


if __name__ == "__main__":
    sys.exit(main())
