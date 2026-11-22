"""Mock-only memory benchmark tests; no real benchmark processes are run."""
import contextlib
import hashlib
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

import measure_mem as mem


STATS = (b"        0.01 real         0.00 user         0.00 sys\n"
         b"             4096  maximum resident set size\n"
         b"                0  page faults\n"
         b"             8192  peak memory footprint\n")


class PeakRssTests(unittest.TestCase):
    def setUp(self):
        system = patch.object(mem.sys, "platform", "darwin")
        system.start()
        self.addCleanup(system.stop)

    def measure(self, stdout=b"42\n", stderr=STATS, code=0, expected="42"):
        proc = subprocess.CompletedProcess([], code, stdout, stderr)
        with patch.object(mem.subprocess, "run", return_value=proc) as run:
            result = mem.peak_rss(["/fake/candy", "case.candy"], expected, 2.5)
        self.assertEqual(run.call_args.kwargs["timeout"], 2.5)
        self.assertEqual(run.call_args.kwargs["env"]["LC_ALL"], "C")
        return result

    def test_time_statistics_are_not_diagnostics(self):
        self.assertEqual(self.measure(), (4096, 8192))

    def test_all_known_time_stat_lines(self):
        stats = b"0.01 real 0.00 user 0.00 sys\n" + b"".join(
            f"4096 {label}\n".encode() for label in mem.TIME_LABELS)
        self.assertEqual(self.measure(stderr=stats), (4096, 4096))

    def test_missing_optional_footprint(self):
        self.assertEqual(self.measure(stderr=STATS.replace(
            b"             8192  peak memory footprint\n", b"")), (4096, 0))

    def test_empty_case_requires_empty_stdout(self):
        self.assertEqual(self.measure(stdout=b"", expected=""), (4096, 8192))
        with self.assertRaises(SystemExit):
            self.measure(stdout=b"\n", expected="")

    def test_exit_zero_incorrect_full_stdout_is_rejected(self):
        for stdout in (b"42", b"42\nextra\n", b"prefix\n42\n", b"43\n", b"42\r\n"):
            with self.subTest(stdout=stdout), self.assertRaises(SystemExit):
                self.measure(stdout=stdout)

    def test_exit_zero_diagnostics_are_rejected(self):
        for diagnostic in (b"compile error\n", b"123 compiler failure\n",
                           b"error: 4096 maximum resident set size\n",
                           b"4096 maximum resident set size warning\n"):
            with self.subTest(diagnostic=diagnostic), self.assertRaises(SystemExit):
                self.measure(stderr=diagnostic + STATS)

    def test_nonzero_even_with_correct_output_is_rejected(self):
        with self.assertRaises(SystemExit):
            self.measure(code=1)

    def test_missing_or_duplicate_statistics_are_rejected(self):
        for stderr in (b"", b"4096 maximum resident set size\n",
                       STATS.replace(b"4096", b"0"),
                       STATS + b"4096 maximum resident set size\n"):
            with self.subTest(stderr=stderr), self.assertRaises(SystemExit):
                self.measure(stderr=stderr)

    def test_timeout_and_launch_error(self):
        for error in (subprocess.TimeoutExpired("fake", 2.5), OSError("unavailable")):
            with self.subTest(error=error), patch.object(mem.subprocess, "run", side_effect=error):
                with self.assertRaises(SystemExit):
                    mem.peak_rss(["fake"], "", 2.5)

    def test_gnu_time_converts_kib_to_bytes(self):
        with patch.object(mem.sys, "platform", "linux"):
            self.assertEqual(mem.time_args(), ["-f", "candy_bench_rss_kib=%M"])
            self.assertEqual(self.measure(stderr=b"candy_bench_rss_kib=4242\n"), (4242 * 1024, 0))

    def test_gnu_time_requires_one_positive_stat_and_no_diagnostics(self):
        line = b"candy_bench_rss_kib=4242\n"
        with patch.object(mem.sys, "platform", "linux"):
            for stderr in (b"", b"candy_bench_rss_kib=0\n", line * 2, b"compiler error\n" + line):
                with self.subTest(stderr=stderr), self.assertRaises(SystemExit):
                    self.measure(stderr=stderr)


class MainTests(unittest.TestCase):
    def setUp(self):
        time_args = patch.object(mem, "time_args", return_value=["-l"])
        time_args.start()
        self.addCleanup(time_args.stop)

    def test_defaults_medians_samples_metadata_and_fresh_case_directory(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            candy = root / "custom-candy"
            lua = root / "lua"
            candy.write_bytes(b"custom-binary")
            lua.write_bytes(b"lua-binary")
            cases_dir = root / "new" / "cases"
            case = dict(name="fixture", candy="print(42)\n", lua="print(42)\n", expected="42")
            # Warmup (999) must not be included; subsequent peaks are unsorted.
            values = [(999, 999), (999, 999), (30, 300), (10, 100),
                      (20, 200), (40, 400), (50, 500), (60, 600)]
            with (patch.object(mem, "ROOT", root),
                  patch.object(mem, "CASES_DIR", cases_dir),
                  patch.object(mem, "CASES", ["fixture"]),
                  patch.object(mem, "build", return_value=[case]),
                  patch.object(mem, "executable", side_effect=[candy, lua]),
                  patch.object(mem, "peak_rss", side_effect=values) as measure,
                  patch.object(sys, "argv", ["measure_mem", "--candy", str(candy),
                                            "--lua", str(lua)]),
                  contextlib.redirect_stdout(io.StringIO())):
                mem.main()
            result = json.loads((root / "memory.json").read_text())
            self.assertEqual((cases_dir / "fixture.candy").read_text(), case["candy"])
            self.assertEqual((cases_dir / "fixture.lua").read_text(), case["lua"])
            # Alternating order: measured Lua/Candy, Candy/Lua, Lua/Candy.
            self.assertEqual(result["fixture"]["candy"]["rss_bytes"], 20)
            self.assertEqual(result["fixture"]["lua"]["rss_bytes"], 40)
            self.assertEqual(result["fixture"]["candy"]["peak_bytes"], 200)
            self.assertEqual(len(result["fixture"]["candy"]["samples"]), 3)
            self.assertEqual(measure.call_count, 8)
            self.assertTrue(all(call.args[1:] == ("42", 120.0)
                                for call in measure.call_args_list))
            metadata = result["_metadata"]
            self.assertEqual(metadata["config"]["reps"], 3)
            self.assertEqual(metadata["config"]["warmup"], 1)
            self.assertEqual(metadata["binaries"]["candy"]["sha256"],
                             hashlib.sha256(b"custom-binary").hexdigest())
            self.assertEqual(result["_sizes"], {"candy_bin": 13, "lua_bin": 10})
            self.assertNotIn(str(Path.home()), json.dumps(metadata))

    def test_cli_overrides(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            binary = root / "binary"
            binary.write_bytes(b"exe")
            case = dict(name="empty", candy="", lua="", expected="")
            with (patch.object(mem, "ROOT", root), patch.object(mem, "CASES_DIR", root / "cases"),
                  patch.object(mem, "CASES", ["empty"]), patch.object(mem, "build", return_value=[case]),
                  patch.object(mem, "executable", return_value=binary),
                  patch.object(mem, "peak_rss", return_value=(100, 200)) as measure,
                  patch.object(sys, "argv", ["measure_mem", "--out", "memory_custom.json",
                                            "--reps", "1", "--warmup", "0", "--timeout", "3"]),
                  contextlib.redirect_stdout(io.StringIO())):
                mem.main()
            self.assertTrue((root / "memory_custom.json").is_file())
            self.assertEqual(measure.call_count, 2)
            self.assertEqual(measure.call_args.args[1:], ("", 3.0))

    def test_invalid_cli_values(self):
        for option, value in (("--reps", "0"), ("--reps", "-1"), ("--warmup", "-1"),
                              ("--timeout", "0"), ("--timeout", "nan"), ("--timeout", "inf")):
            with (self.subTest(option=option, value=value),
                  patch.object(sys, "argv", ["measure_mem", option, value]),
                  contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit)):
                mem.main()


if __name__ == "__main__":
    unittest.main()
