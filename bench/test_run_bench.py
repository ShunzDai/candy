"""Validation regressions; run with python3 -m unittest discover -s bench."""
import argparse
from contextlib import redirect_stdout
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

# Support both unittest discovery and loading this file directly by path.
BENCH = Path(__file__).resolve().parent
sys.path.insert(0, str(BENCH))
import run_bench as bench


class ValidationTests(unittest.TestCase):
    def setUp(self):
        self.case = {"name": "value", "expected": "42"}

    def check(self, out="42\n", err="", code=0, expected="42", phase="verify"):
        case = dict(self.case, expected=expected)
        with mock.patch.object(bench, "run_once", return_value=(code, out, err, 123)):
            return bench.checked_run("candy", case, phase, 0.5)

    def test_exact_stdout(self):
        self.assertEqual(self.check(), 123)
        self.assertEqual(self.check(out="", expected=""), 123)
        for out in ("42", "42\n\n", " 42\n", "42 \n", "42\r\n", "extra\n42\n"):
            with self.subTest(out=out), self.assertRaisesRegex(SystemExit, "verify failed"):
                self.check(out=out)
        for out in ("\n", " ", "unexpected"):
            with self.subTest(out=out), self.assertRaises(SystemExit):
                self.check(out=out, expected="")

    def test_all_phases_reject_stderr_and_nonzero(self):
        for phase in ("verify", "warmup", "run"):
            for invalid in ({"err": "warning\n"}, {"err": " "}, {"code": 1}, {"code": -9}):
                with self.subTest(phase=phase, invalid=invalid):
                    with self.assertRaisesRegex(SystemExit, f"{phase} failed"):
                        self.check(phase=phase, **invalid)

    def test_verify_checks_both_languages_and_forwards_timeout(self):
        with mock.patch.object(bench, "checked_run") as checked:
            bench.verify([self.case], 0.25)
        self.assertEqual(checked.call_args_list, [
            mock.call("candy", self.case, "verify", 0.25),
            mock.call("lua", self.case, "verify", 0.25),
        ])

    def test_every_warmup_pass_runs_both_languages(self):
        cases = [self.case, {"name": "empty", "expected": ""}]
        for passes in (0, 1, 2, 3, 4):
            with self.subTest(passes=passes), mock.patch.object(bench, "checked_run") as checked:
                bench.warmup(cases, passes, 0.5)
                expected = []
                for w in range(passes):
                    order = ("candy", "lua") if w % 2 == 0 else ("lua", "candy")
                    for case in cases:
                        expected.extend(mock.call(lang, case, "warmup", 0.5) for lang in order)
                self.assertEqual(checked.call_args_list, expected)

    def test_verify_and_warmup_reject_stderr(self):
        for phase in ("verify", "warmup"):
            with self.subTest(phase=phase):
                with mock.patch.object(bench, "run_once", return_value=(0, "42\n", "warning", 1)):
                    with self.assertRaisesRegex(SystemExit, f"{phase} failed"):
                        if phase == "verify":
                            bench.verify([self.case])
                        else:
                            bench.warmup([self.case], 2, 1)

    def test_run_once_real_process(self):
        code, out, err, elapsed = bench.run_once(
            [sys.executable, "-c", "import sys; print('42'); sys.stderr.write('warning'); sys.exit(3)"], 5)
        self.assertEqual((code, out, err), (3, "42\n", "warning"))
        self.assertGreater(elapsed, 0)

    def test_timeout_is_bounded_and_readable(self):
        with self.assertRaisesRegex(SystemExit, "timed out after 0.05s"):
            bench.run_once([sys.executable, "-c", "import time; time.sleep(10)"], 0.05)

    def test_missing_executable_is_readable(self):
        with tempfile.TemporaryDirectory() as tmp:
            missing = str(Path(tmp) / "not-an-executable")
            with self.assertRaisesRegex(SystemExit, "cannot run"):
                bench.run_once([missing], 1)
            with self.assertRaisesRegex(SystemExit, "lua executable not found"):
                bench.executable(missing, "lua")
            with self.assertRaisesRegex(SystemExit, "not executable"):
                bench.executable(tmp, "candy")
            file = Path(tmp) / "not-executable"
            file.write_text("not a program")
            with self.assertRaisesRegex(SystemExit, "not executable"):
                bench.executable(str(file), "candy")

    def test_positive_arguments(self):
        self.assertEqual(bench.positive_int("2"), 2)
        self.assertEqual(bench.nonnegative_int("0"), 0)
        self.assertEqual(bench.positive_timeout("0.25"), 0.25)
        for value in ("0", "-1"):
            with self.subTest(reps=value), self.assertRaises(argparse.ArgumentTypeError):
                bench.positive_int(value)
        with self.assertRaises(argparse.ArgumentTypeError):
            bench.nonnegative_int("-1")
        for value in ("0", "-1", "nan", "inf", "-inf"):
            with self.subTest(timeout=value), self.assertRaises(argparse.ArgumentTypeError):
                bench.positive_timeout(value)

    def test_home_paths_are_sanitized(self):
        home = str(Path.home())
        self.assertEqual(bench.sanitize(f"{home}/bin/candy -> {home}/bench/results.json"),
                         "~/bin/candy -> ~/bench/results.json")
        with mock.patch.object(bench, "CANDY", Path(home) / "bin/candy"):
            with mock.patch.object(bench, "run_once", return_value=(1, "", home + "/warning", 1)):
                with self.assertRaises(SystemExit) as failure:
                    bench.checked_run("candy", self.case, "run", 1)
                self.assertNotIn(home, str(failure.exception))

    @mock.patch.object(bench.platform, "platform", return_value="test-platform")
    @mock.patch.object(bench.platform, "processor", return_value="test-processor")
    def test_env_has_no_stale_build_release_sizes_and_bounds_probes(self, _processor, _platform):
        home = Path.home()
        proc = subprocess.CompletedProcess([], 0, "Lua test\n", "")
        with mock.patch.object(bench, "CANDY", home / "bin/candy"), \
                mock.patch.object(bench, "LUA", str(home / "bin/lua")), \
                mock.patch.object(bench.subprocess, "run", return_value=proc) as run:
            info = bench.env_info(0.5)
        self.assertEqual(info["candy"], "~/bin/candy")
        self.assertEqual(info["lua"], "~/bin/lua")
        self.assertNotIn("candy_core_bytes", info)
        self.assertNotIn("candy_builtin_bytes", info)
        for call in run.call_args_list:
            self.assertEqual(call.kwargs["timeout"], 0.5)
        with mock.patch.object(bench.subprocess, "run", side_effect=subprocess.TimeoutExpired([], 1)):
            self.assertNotIn("lua_version", bench.env_info(0.5))


class MainTests(unittest.TestCase):
    def run_main(self, directory, extra=(), outcome=None):
        root = Path(directory)
        cases = [dict(name="empty", group="startup", expected="", candy="", lua="",
                      work=0, unit="", reps=2)]
        argv = ["run_bench.py", "--candy", sys.executable, "--lua", sys.executable,
                "--reps", "2", "--warmup", "3", "--timeout", "0.5", *extra]
        with mock.patch.object(sys, "argv", argv), \
                mock.patch.object(bench, "ROOT", root), \
                mock.patch.object(bench, "CASES_DIR", root / "cases"), \
                mock.patch.object(bench, "build", return_value=cases), \
                mock.patch.object(bench, "CANDY", bench.CANDY), \
                mock.patch.object(bench, "LUA", bench.LUA), \
                mock.patch.object(bench, "env_info", return_value={}), \
                mock.patch.object(bench, "run_once", side_effect=outcome,
                                  return_value=(0, "", "", 1000)) as run, \
                redirect_stdout(io.StringIO()):
            bench.main()
        return run

    def test_end_to_end_counts_schema_and_explicit_lua(self):
        with tempfile.TemporaryDirectory() as tmp:
            run = self.run_main(tmp)
            result = json.loads((Path(tmp) / "results.json").read_text())
            self.assertTrue((Path(tmp) / "results.csv").is_file())
        # Both languages: one verification, three warmups, two timed repetitions.
        self.assertEqual(run.call_count, 12)
        for call in run.call_args_list:
            self.assertEqual(call.args[0][0], str(Path(sys.executable).resolve()))
            self.assertEqual(call.args[1], 0.5)
        self.assertEqual(set(result), {"env", "measurement", "rows", "raw_ns"})
        self.assertEqual(result["measurement"]["warmup_passes"], 3)
        self.assertEqual(result["raw_ns"]["empty"], {"candy": [1000, 1000], "lua": [1000, 1000]})
        self.assertEqual(result["rows"][0]["candy"]["n"], 2)

    def test_timed_run_validation_even_when_verification_skipped(self):
        for result in ((0, "", "warning", 1), (1, "", "", 1), (0, "\n", "", 1)):
            with self.subTest(result=result), tempfile.TemporaryDirectory() as tmp:
                with self.assertRaisesRegex(SystemExit, "run failed"):
                    self.run_main(tmp, ["--skip-verify", "--warmup", "0"], outcome=[result])
                self.assertFalse((Path(tmp) / "results.json").exists())

    def test_cli_rejects_invalid_numbers_before_building(self):
        for option, value in (("--reps", "0"), ("--reps", "-1"), ("--warmup", "-1"),
                              ("--timeout", "0"), ("--timeout", "nan"), ("--timeout", "inf")):
            with self.subTest(option=option, value=value), \
                    mock.patch.object(sys, "argv", ["run_bench.py", option, value]), \
                    mock.patch.object(bench, "build") as build, \
                    mock.patch("sys.stderr", new_callable=io.StringIO):
                with self.assertRaises(SystemExit) as failure:
                    bench.main()
                self.assertEqual(failure.exception.code, 2)
                build.assert_not_called()


if __name__ == "__main__":
    unittest.main()
