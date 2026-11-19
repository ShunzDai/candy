"""Ensure the published time and memory data describe one candidate."""
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parent

PLOTTED = ['fib_34', 'fib_alias_30', 'calls_400k', 'calls_local_200k',
           'mutual_10k', 'globals_400k', 'arith_400k', 'compile_200k', 'closures_100k',
           'calls_nested_local_200k', 'while_global_1m', 'while_local_1m', 'for_local_500k',
           'calls_hot_global_500k', 'calls_hot_local_500k', 'calls_hot_parameter_500k',
           'branch_chain_500k', 'break_continue_500k', 'tail_self_100k',
           'nested_functions_1k']


class ChartInputTests(unittest.TestCase):
    def test_time_data_has_all_plotted_cases(self):
        data = json.loads((ROOT / 'final_register.json').read_text())
        rows = {r['name']: r for r in data['rows']}
        for name in PLOTTED:
            self.assertIn(name, rows)
            self.assertGreater(rows[name]['ratio_median'], 0)
        self.assertEqual(len(rows), 35)
        self.assertGreaterEqual(data['measurement']['warmup_passes'], 2)
        self.assertEqual(rows['calls_local_200k']['binding'], 'asymmetric')
        self.assertEqual(rows['calls_nested_local_200k']['binding'], 'aligned')
        self.assertEqual(rows['nested_functions_1k']['binding'], 'asymmetric')
        for name in ('calls_hot_global_500k', 'calls_hot_local_500k', 'calls_hot_parameter_500k'):
            self.assertEqual(rows[name]['measurement_kind'], 'hot_loop')
            self.assertGreater(rows[name]['net_candy_ms'], 0)

    def test_memory_and_time_have_same_candidate(self):
        memory = json.loads((ROOT / 'memory_register.json').read_text())
        time = json.loads((ROOT / 'final_register.json').read_text())
        self.assertEqual(memory['_metadata']['binaries']['candy']['sha256'],
                         time['env']['candy_sha256'])
        rows = {r['name'] for r in time['rows']}
        for name in memory:
            if name.startswith('_'):
                continue
            self.assertIn(name, rows)
            self.assertGreater(memory[name]['candy']['peak_bytes'], 0)
            self.assertEqual(len(memory[name]['candy']['samples']), 3)
