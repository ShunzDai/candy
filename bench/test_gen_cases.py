"""Generator-only regressions; no Candy builds or benchmark timing runs.

Run: python3 -m unittest discover -s bench -p test_gen_cases.py
If luac is installed, all generated Lua sources are syntax-checked as well.
"""
import shutil
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

BENCH = Path(__file__).resolve().parent
sys.path.insert(0, str(BENCH))
import gen_cases as gen


class GeneratorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cases = gen.build()
        cls.by_name = {case["name"]: case for case in cls.cases}

    def test_names_unique_and_legacy_names_preserved(self):
        self.assertEqual(len(self.by_name), len(self.cases))
        legacy = {"empty", "fib_global_32", "fib_alias_30", "calls_local_200k",
                  "mutual_10k", "deep_60", "closures_100k"}
        legacy.update(f"fib_{n}" for n in (28, 30, 32, 34))
        legacy.update(f"{group}_{n}k" for group in ("calls", "globals", "arith")
                      for n in (100, 200, 400))
        legacy.update(f"compile_{n}k" for n in (50, 100, 200))
        self.assertTrue(legacy <= self.by_name.keys())
        self.assertEqual(len(self.cases) - len(legacy), 12)

    def test_expected_values_and_new_reps(self):
        expected = {
            "calls_nested_local_200k": 1,
            "while_global_1m": 499999500000,
            "while_local_1m": 499999500000,
            "for_local_500k": 124999750000,
            "calls_hot_global_500k": 124999750000,
            "calls_hot_local_500k": 124999750000,
            "calls_hot_parameter_500k": 124999750000,
            "branch_chain_500k": 1000001,
            "break_continue_500k": 41666583333,
            "tail_self_100k": 5000050000,
            "nested_functions_1k": 499500,
        }
        for name, value in expected.items():
            with self.subTest(name=name):
                self.assertEqual(self.by_name[name]["expected"], str(value))
                self.assertEqual(self.by_name[name]["reps"], 7)
        for n, value in ((28, 317811), (30, 832040), (32, 2178309), (34, 5702887)):
            self.assertEqual(self.by_name[f"fib_{n}"]["expected"], str(value))

    def test_independent_reference_calculations(self):
        # Smaller uneven sizes catch boundary and inclusive/exclusive mistakes.
        for n in (1, 2, 7, 19):
            self.assertEqual(n * (n - 1) // 2, sum(range(n)))
            first, second = n // 3, 2 * n // 3
            self.assertEqual(first + 2 * (second - first) + 3 * (n - second),
                             sum(1 if i < first else 2 if i < second else 3 for i in range(n)))
            total = 0
            for i in range(n):
                if i < first:
                    continue
                if second - 1 < i:
                    break
                total += i
            self.assertEqual(second * (second - 1) // 2 - first * (first - 1) // 2, total)

    def test_metadata_and_legacy_asymmetry(self):
        for case in self.cases:
            self.assertIn(case["measurement_kind"],
                          ("end_to_end", "hot_loop"))
            self.assertIn(case["binding"], ("aligned", "asymmetric"))
        legacy = self.by_name["calls_local_200k"]
        expected = gen.render("calls", body=gen.body(["id(1)"] * 200_000),
                              func="local function")
        self.assertEqual((legacy["candy"], legacy["lua"]), expected)
        self.assertEqual(legacy["binding"], "asymmetric")
        self.assertIn("global id", legacy["binding_note"])
        self.assertEqual(self.by_name["calls_nested_local_200k"]["binding"], "aligned")
        for name in ("fib_28", "deep_60", "while_local_1m", "tail_self_100k"):
            self.assertEqual(self.by_name[name]["measurement_kind"], "hot_loop")

    def test_only_supported_candy_operators(self):
        for case in self.cases:
            with self.subTest(name=case["name"]):
                self.assertNotRegex(case["candy"], r"[*/%>!&|^]|==|<=")
                self.assertNotIn("local ", case["candy"])

    def test_true_locals_and_dynamic_call_sources(self):
        nested = self.by_name["calls_nested_local_200k"]
        self.assertTrue(nested["candy"].startswith("def run()\n  def id(n)"))
        self.assertTrue(nested["lua"].startswith("local function run()\n  local function id(n)"))
        self.assertEqual(nested["candy"].count("id(1)"), 200_001)
        for binding in ("global", "local", "parameter"):
            case = self.by_name[f"calls_hot_{binding}_500k"]
            self.assertLess(len(case["candy"]), 500)
            self.assertIn("local i = 0", case["lua"])
            self.assertIn("local sum = 0", case["lua"])
            self.assertIn("sum = sum + " + ("callee(i)" if binding == "parameter" else "id(i)"),
                          case["candy"])
        local = self.by_name["calls_hot_local_500k"]
        self.assertLess(local["candy"].index("def run()"), local["candy"].index("def id(n)"))
        self.assertIn("def run(callee)", self.by_name["calls_hot_parameter_500k"]["candy"])

    def test_fair_lowered_for_and_continue_step(self):
        case = self.by_name["for_local_500k"]
        self.assertIn("while (i < 500000)", case["candy"])
        self.assertIn("while i < 500000 do", case["lua"])
        self.assertNotRegex(case["lua"], r"\bfor\b")
        case = self.by_name["break_continue_500k"]
        self.assertIn("continue", case["candy"])
        self.assertIn("break", case["candy"])
        self.assertRegex(case["lua"], r"end\n  ::continue_loop::\n  i = i \+ 1")
        self.assertIn("  do\n", case["lua"])
        self.assertIn("goto continue_loop", case["lua"])

    @unittest.skipUnless(shutil.which("luac"), "optional Lua compiler unavailable")
    def test_lua_reference_syntax(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "case.lua"
            for case in self.cases:
                with self.subTest(name=case["name"]):
                    path.write_text(case["lua"])
                    result = subprocess.run([shutil.which("luac"), "-p", str(path)],
                                            capture_output=True, text=True, timeout=15)
                    self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == "__main__":
    unittest.main()
