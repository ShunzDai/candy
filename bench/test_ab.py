"""Tests for paired A/B measurement validation."""
import unittest
from pathlib import Path
from unittest.mock import patch

import run_ab


class ABValidationTests(unittest.TestCase):
    def test_valid_output(self):
        with patch.object(run_ab, 'run_once', return_value=(0, '42\n', '', 123)):
            self.assertEqual(run_ab.checked(Path('candy'), Path('case'), {'name': 'case', 'expected': '42'}), 123)

    def test_rejects_successful_exit_with_diagnostic(self):
        for result in [(0, '', 'missing input', 1), (0, '42\n\n', '', 1), (1, '42\n', '', 1)]:
            with self.subTest(result=result), patch.object(run_ab, 'run_once', return_value=result):
                with self.assertRaises(RuntimeError):
                    run_ab.checked(Path('candy'), Path('case'), {'name': 'case', 'expected': '42'})

    def test_empty_case_requires_empty_output(self):
        with patch.object(run_ab, 'run_once', return_value=(0, '\n', '', 1)):
            with self.assertRaises(RuntimeError):
                run_ab.checked(Path('candy'), Path('case'), {'name': 'empty', 'expected': ''})
