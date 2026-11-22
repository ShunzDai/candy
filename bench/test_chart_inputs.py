"""Check paired time and memory inputs for the two published environments."""
import json
from pathlib import Path
from statistics import median
import unittest

ROOT = Path(__file__).resolve().parent

PLOTTED = ['fib_34', 'fib_global_32', 'fib_alias_30', 'calls_200k', 'calls_400k',
           'mutual_10k', 'globals_400k', 'arith_400k', 'compile_200k', 'closures_100k',
           'calls_nested_local_200k', 'while_global_1m', 'while_local_1m', 'for_local_500k',
           'calls_hot_global_500k', 'calls_hot_local_500k', 'calls_hot_parameter_500k',
           'branch_chain_500k', 'break_continue_500k', 'tail_self_100k',
           'nested_functions_1k']


class ChartInputTests(unittest.TestCase):
    def test_time_data_has_all_plotted_cases(self):
        data = json.loads((ROOT / 'macos_time.json').read_text())
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
        memory = json.loads((ROOT / 'macos_memory.json').read_text())
        time = json.loads((ROOT / 'macos_time.json').read_text())
        for lang in ('candy', 'lua'):
            self.assertEqual(memory['_metadata']['binaries'][lang]['sha256'],
                             time['env'][f'{lang}_sha256'])
        rows = {r['name'] for r in time['rows']}
        for name in memory:
            if name.startswith('_'):
                continue
            self.assertIn(name, rows)
            self.assertGreater(memory[name]['candy']['peak_bytes'], 0)
            self.assertEqual(len(memory[name]['candy']['samples']), 3)

    def test_wsl_time_has_matching_cases_and_complete_samples(self):
        data = json.loads((ROOT / 'wsl_time.json').read_text())
        macos = json.loads((ROOT / 'macos_time.json').read_text())
        rows = {r['name']: r for r in data['rows']}
        self.assertEqual(set(rows), {r['name'] for r in macos['rows']})
        self.assertIn('wsl2', data['env']['platform'].lower())
        self.assertEqual(data['measurement']['warmup_passes'], 3)
        self.assertEqual(data['measurement']['reps_override'], 15)
        for name, row in rows.items():
            with self.subTest(case=name):
                for lang in ('candy', 'lua'):
                    samples = data['raw_ns'][name][lang]
                    self.assertEqual(len(samples), 15)
                    self.assertEqual(row[lang]['median_ms'], median(samples) / 1e6)
                self.assertAlmostEqual(row['ratio_median'],
                                       row['candy']['median_ms'] / row['lua']['median_ms'])

    def test_wsl_memory_has_matching_cases_and_binary_fingerprints(self):
        memory = json.loads((ROOT / 'wsl_memory.json').read_text())
        macos = json.loads((ROOT / 'macos_memory.json').read_text())
        time = json.loads((ROOT / 'wsl_time.json').read_text())
        cases = {name for name in memory if not name.startswith('_')}
        self.assertEqual(cases, {name for name in macos if not name.startswith('_')})
        metadata = memory['_metadata']
        self.assertIn('wsl2', metadata['platform'].lower())
        for key in ('reps', 'warmup'):
            self.assertEqual(metadata['config'][key], macos['_metadata']['config'][key])
        for lang in ('candy', 'lua'):
            self.assertEqual(metadata['binaries'][lang]['sha256'], time['env'][f'{lang}_sha256'])
            for name in cases:
                with self.subTest(lang=lang, case=name):
                    samples = [s['rss_bytes'] for s in memory[name][lang]['samples']]
                    self.assertEqual(len(samples), 3)
                    self.assertTrue(all(rss > 0 and rss % 1024 == 0 for rss in samples))
                    self.assertEqual(memory[name][lang]['rss_bytes'], median(samples))
