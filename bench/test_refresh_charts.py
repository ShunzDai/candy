import subprocess
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory
from unittest.mock import patch

from refresh_charts import environment, refresh


class RefreshChartTests(unittest.TestCase):
    def test_platform_outputs_are_distinct(self):
        self.assertEqual(environment('Darwin', '25.6.0', 'arm64')[0], 'macos')
        self.assertEqual(environment('Linux', '6.18-microsoft-standard-WSL2', 'x86_64')[0], 'wsl')
        self.assertEqual(environment('Linux', '4.4-Microsoft', 'x86_64')[0], 'wsl1-x86_64')
        self.assertEqual(environment('Linux', '6.12', 'x86_64')[0], 'linux-x86_64')
        self.assertEqual(environment('FreeBSD', '14.3', 'arm64')[0], 'freebsd-arm64')

    @staticmethod
    def measure(argv, *, check):
        assert check
        output = Path(argv[argv.index('--out') + 1])
        output.write_text('new data')
        if output.name.endswith('_time.json'):
            output.with_suffix('.csv').write_text('new csv')

    @staticmethod
    def render(datasets, out, font):
        for entry in datasets:
            for name in entry[4:]:
                (out / name).write_text('new chart')

    def test_refresh_preserves_peer_measurements(self):
        with TemporaryDirectory() as tmp:
            root = Path(tmp)
            for name in ('wsl_time.json', 'wsl_memory.json'):
                (root / name).write_text('old data')
            with patch('refresh_charts.subprocess.run', side_effect=self.measure):
                refresh(root, environment('Darwin', '', 'arm64'), 'candy', 'lua', None, self.render)
            self.assertEqual((root / 'wsl_time.json').read_text(), 'old data')
            self.assertEqual((root / 'wsl_memory.json').read_text(), 'old data')
            self.assertEqual((root / 'macos_time.csv').read_text(), 'new csv')
            self.assertTrue((root / 'charts/wsl_candy_vs_lua_time.png').is_file())

    def test_failed_measurement_does_not_replace_previous_data(self):
        with TemporaryDirectory() as tmp:
            root = Path(tmp)
            target = root / 'macos_time.json'
            target.write_text('old data')
            with patch('refresh_charts.subprocess.run', side_effect=subprocess.CalledProcessError(1, 'measure')):
                with self.assertRaises(subprocess.CalledProcessError):
                    refresh(root, environment('Darwin', '', 'arm64'), 'candy', 'lua', None, self.render)
            self.assertEqual(target.read_text(), 'old data')

    def test_new_platform_does_not_replace_published_charts(self):
        with TemporaryDirectory() as tmp:
            root = Path(tmp)
            charts = root / 'charts'
            charts.mkdir()
            previous = charts / 'macos_candy_vs_lua_time.png'
            previous.write_text('old chart')
            with patch('refresh_charts.subprocess.run', side_effect=self.measure):
                refresh(root, environment('Linux', '6.12', 'x86_64'), 'candy', 'lua', None, self.render)
            self.assertEqual(previous.read_text(), 'old chart')
            self.assertTrue((root / 'linux-x86_64_time.json').is_file())
            self.assertTrue((charts / 'linux-x86_64_candy_vs_lua_time.png').is_file())
