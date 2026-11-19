#!/usr/bin/env python3
"""Generate the Candy/Lua benchmark cases.

The case sources live in ``bench/scripts`` as template pairs (``$name``
placeholders); this file only decides the parameters (sizes, repetition),
the expected results and the measurement metadata. Templates are expanded
into ``bench/cases`` for the benchmark runner.

Use integer add/sub/less-than, control flow, mutable parameters, function-local
assignments and nested defs; no upvalues or unsupported arithmetic operators.
Legacy binding asymmetries are explicit metadata, not silently changed sources.
Timing includes process startup and compilation even for small-source hot loops.
"""
from pathlib import Path
from string import Template

ROOT = Path(__file__).resolve().parent
CASES = ROOT / "cases"
SCRIPTS = ROOT / "scripts"


def render(name: str, **params: object) -> tuple[str, str]:
    """Expand the ``scripts/<name>.{candy,lua}`` template pair."""
    def one(suffix: str) -> str:
        text = (SCRIPTS / f"{name}.{suffix}").read_text()
        return Template(text).substitute(params)

    return one("candy"), one("lua")


def body(items: list[str]) -> str:
    """Join repeated statements into a single ``$body`` slot."""
    return "\n".join(items)


def fibonacci(n: int) -> int:
    a, b = 0, 1
    for _ in range(n):
        a, b = b, a + b
    return a


def build() -> list[dict]:
    cases: list[dict] = []

    def add(name, group, src, expected, work=0, unit="", reps=7,
            measurement_kind=None, binding="aligned", binding_note=""):
        candy, lua = src
        if measurement_kind is None:
            measurement_kind = ("hot_loop" if group in ("fib", "depth")
                                else "end_to_end")
        cases.append(dict(name=name, group=group, candy=candy, lua=lua,
                          expected=expected, work=work, unit=unit, reps=reps,
                          measurement_kind=measurement_kind, binding=binding,
                          binding_note=binding_note))

    add("empty", "startup", ("", ""), "", reps=31)
    fib_values = {n: fibonacci(n) for n in (28, 30, 32, 34)}
    for n in (28, 30, 32, 34):
        add(f"fib_{n}", "fib", render("fib", n=n), str(fib_values[n]), reps=11,
            binding="asymmetric", binding_note="Candy global self function; Lua local self function.")
    add("fib_global_32", "fib", render("fib_global", n=32), str(fib_values[32]), reps=11)
    add("fib_alias_30", "fib", render("fib_alias", n=30), str(fib_values[30]), reps=11)

    for k in (100_000, 200_000, 400_000):
        add(f"calls_{k // 1000}k", "calls",
            render("calls", body=body(["id(1)"] * k), func="function"),
            "1", work=k + 1, unit="calls", reps=11 if k < 400_000 else 7)
    add("calls_local_200k", "calls",
        render("calls", body=body(["id(1)"] * 200_000), func="local function"),
        "1", work=200_001, unit="calls", reps=11, binding="asymmetric",
        binding_note="Legacy: Candy top-level global id; Lua chunk-local id. See calls_nested_local_200k for aligned function locals.")
    add("mutual_10k", "calls",
        render("mutual", body=body([f"even(40)"] * 10_000), depth=40),
        "1", work=410_001, unit="calls", reps=7)

    for k in (100_000, 200_000, 400_000):
        add(f"globals_{k // 1000}k", "globals",
            render("globals", body=body(["y = x + 1"] * k)),
            "2", work=k, unit="ops", reps=11 if k < 400_000 else 7)

    for k in (100_000, 200_000, 400_000):
        add(f"arith_{k // 1000}k", "arith", render("arith", expr="+".join(["a"] * k)),
            str(k), work=k, unit="adds", reps=11 if k < 400_000 else 7,
            binding="asymmetric",
            binding_note="Candy global f; Lua chunk-local f; arithmetic parameters aligned.")

    for k in (50_000, 100_000, 200_000):
        add(f"compile_{k // 1000}k", "compile",
            render("compile", body=body([f"y = {i} + z" for i in range(k)])),
            "", work=k, unit="lines", reps=9 if k < 200_000 else 7)

    add("deep_60", "depth", render("deep", n=60), str(60), reps=5,
        binding="asymmetric", binding_note="Candy global f; Lua chunk-local f.")
    add("closures_100k", "alloc",
        render("closures",
               candy_body=body([f"def f{i}()\n  return {i}\nend" for i in range(100_000)]),
               lua_body=body([f"function f{i}() return {i} end" for i in range(100_000)]),
               last=100_000 - 1),
        str(100_000 - 1), work=100_000, unit="defs", reps=7)

    add("calls_nested_local_200k", "calls",
        render("nested_local_calls", body=body(["  id(1)"] * 200_000)),
        str(1), work=200_000 + 1, unit="calls",
        binding_note="Both id bindings are nested locals; outer run is Candy global/Lua local.")

    def hot(name, group, src, expected, work, unit="iterations"):
        add(name, group, src, str(expected), work=work, unit=unit,
            measurement_kind="hot_loop",
            binding_note="Measured body bindings aligned; outer run is Candy global/Lua local.")

    million = 1_000_000
    for local in (False, True):
        hot(f"while_{'local' if local else 'global'}_1m", "loops",
            render("while_local" if local else "while_global", k=million),
            million * (million - 1) // 2, million)
    k = 500_000
    checksum = k * (k - 1) // 2
    hot("for_local_500k", "loops", render("for_local", k=k), checksum, k)
    rk = 50_000
    rchecksum = rk * (rk - 1) // 2
    hot("for_range_100k", "loops",
        render("for_range", elems=",".join(str(i) for i in range(rk))), rchecksum, rk)
    for binding in ("global", "local", "parameter"):
        hot(f"calls_hot_{binding}_500k", "calls",
            render(f"calls_hot_{binding}", k=k), checksum, k, "calls")
    first, second = k // 3, 2 * k // 3
    branch_expected = first + 2 * (second - first) + 3 * (k - second)
    hot("branch_chain_500k", "branches",
        render("branch_chain", k=k, first=first, second=second), branch_expected, k)
    # Sum [first, second), independently of the generated break/continue body.
    break_expected = second * (second - 1) // 2 - first * (first - 1) // 2
    hot("break_continue_500k", "loops",
        render("break_continue", k=k, skip=first, stop=second - 1), break_expected, second + 1)
    depth = 100_000
    hot("tail_self_100k", "calls", render("tail_self", k=depth), depth * (depth + 1) // 2, depth + 1, "calls")
    definitions = 1_000
    hot("nested_functions_1k", "alloc", render("nested_creation", k=definitions),
        definitions * (definitions - 1) // 2, definitions, "defs")
    cases[-1]["binding"] = "asymmetric"
    cases[-1]["binding_note"] = "Repeated def binding: Candy reuses a compile-time closure; Lua creates a closure at runtime. Not equivalent allocation pressure."
    return cases


def main() -> None:
    CASES.mkdir(exist_ok=True)
    for case in build():
        (CASES / f"{case['name']}.candy").write_text(case["candy"])
        (CASES / f"{case['name']}.lua").write_text(case["lua"])
    print(f"generated {len(build())} cases in {CASES}")


if __name__ == "__main__":
    main()
