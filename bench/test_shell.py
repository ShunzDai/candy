"""Shell failures must not look like successful benchmark executions.

Run: CANDY_BIN=build/shell/candy python3 -m unittest discover -s bench -p test_shell.py
"""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest


class ShellExitTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.binary = Path(os.environ.get('CANDY_BIN', 'build/shell/candy')).resolve()
        if not cls.binary.is_file():
            raise unittest.SkipTest('build the Candy shell first')

    def execute(self, source):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'input.candy'
            if source is not None:
                path.write_text(source)
            return subprocess.run([str(self.binary), str(path)], capture_output=True, timeout=10)

    def test_success(self):
        result = self.execute('print(42)\n')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, b'42\n')
        self.assertEqual(result.stderr, b'')

    def test_missing_file(self):
        self.assertNotEqual(self.execute(None).returncode, 0)

    def test_syntax_error(self):
        self.assertNotEqual(self.execute('def broken(\n').returncode, 0)
