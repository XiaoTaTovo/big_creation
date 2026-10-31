"""End-to-end checks of the real compiled CLI and the firmware's existing C tests."""
import csv
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

EXE, MODEL, GCC = map(Path, sys.argv[1:4])
ROOT = Path(__file__).resolve().parents[1]


class ReplayTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='pd-replay-')
        self.addCleanup(self.temp.cleanup)
        self.work = Path(self.temp.name)

    def run_cli(self, *args, stdin='', ok=True):
        result = subprocess.run([str(EXE), *map(str, args)], input=stdin,
                                capture_output=True, text=True)
        self.assertEqual(result.returncode == 0, ok, result.stdout + result.stderr)
        return result

    def trace(self, *args):
        path = self.work / 'trace.csv'
        self.run_cli(*args, '--export', path)
        with path.open(newline='') as f:
            return list(csv.DictReader(f))

    def fixture(self):
        path = self.work / 'events.csv'
        self.run_cli('--auto', '--save-events', path)
        return path

    def test_normal_fields_and_phases(self):
        rows = self.trace()
        self.assertEqual([r['phase'] for r in rows],
                         ['HAVE_CAPS'] * 2 + ['REQUESTED'] * 2 + ['ACCEPTED'] * 2 + ['READY'] * 2)
        self.assertEqual([r['frames'] for r in rows], list(map(str, range(1, 9))))
        self.assertEqual([r['t8_raw'] for r in rows], ['5','6','9','9','11','11','166','167'])
        for r in rows[:2]:
            self.assertEqual(r['request_valid'], '0')
            self.assertEqual(r['voltage_mv'], '')
        for r in rows[2:]:
            self.assertEqual((r['pdo_index'], r['voltage_mv'], r['operating_ma'], r['maximum_ma']),
                             ('3', '12000', '1670', '1670'))
        self.assertTrue(all(r['bad_frames'] == '0' for r in rows))
        self.assertEqual(rows[-1]['message'], 'GoodCRC')

    def test_interactive_reset_end_and_quit(self):
        result = self.run_cli(stdin='\n' * 9 + 'r\na\nq\n').stdout
        self.assertIn('End of replay', result)
        self.assertEqual(result.count('Frames: 8, CRC/length bad: 0'), 2)
        self.assertEqual(result.count('Phase: WAITING'), 2)
        self.assertNotIn('#9 ', result)

    def test_bad_request_not_ready(self):
        rows = self.trace('--bad-request')
        self.assertEqual(rows[-1]['phase'], 'HAVE_CAPS')
        self.assertEqual(rows[-1]['bad_frames'], '1')
        self.assertEqual(rows[-1]['request_valid'], '0')
        self.assertTrue(all(r['voltage_mv'] == '' for r in rows))
        # Injection must not mutate the built-in fixture for the next process.
        self.assertIn('Phase: READY', self.run_cli('--auto').stdout)

    def test_csv_roundtrip(self):
        path = self.fixture()
        rows = self.trace('--input', path)
        self.assertEqual(len(rows), 8)
        self.assertEqual(rows[-1]['voltage_mv'], '12000')
        self.assertEqual(rows[-1]['phase'], 'READY')

    def test_protect_existing_files(self):
        path = self.fixture()
        original = path.read_bytes()
        self.run_cli('--auto', '--save-events', path, ok=False)
        self.run_cli('--input', path, '--export', path, ok=False)
        self.assertEqual(path.read_bytes(), original)

    def test_chinese_filename(self):
        path = self.work / '回放测试.csv'
        self.run_cli('--auto', '--save-events', path)
        self.assertTrue(path.exists())
        self.assertIn('Phase: READY', self.run_cli('--auto', '--input', path).stdout)

    def test_reject_malformed_csv(self):
        original = self.fixture().read_text()
        variants = [
            '', original.splitlines()[0] + '\n',
            original.replace('Source_Capabilities', 'UnknownMessage', 1),
            original.replace('SRC,0,5,1,1,5', 'SRC,8,5,1,1,5', 1),
            original.replace('SRC,0,5,1,1,5', 'SRC,0,5,1,1,6', 1),
            original.replace('0801912C', '100000000', 1),
            original.replace('0801912C', '', 1),
            original.replace('SRC,0,5', 'SRC,-1,5', 1),
            original.replace('GoodCRC,SNK,0,6,1,1,0,00000000', 'GoodCRC,SNK,0,6,1,1,0,00000001', 1),
            original + 'x' * 600,
        ]
        for index, content in enumerate(variants):
            with self.subTest(index=index):
                path = self.work / f'bad-{index}.csv'
                path.write_text(content)
                result = self.run_cli('--auto', '--input', path, ok=False)
                self.assertIn('Invalid event CSV', result.stderr)

    def test_input_file_changes_state(self):
        path = self.fixture()
        text = path.read_text().replace('Request,SNK,0,9,1,1,1', 'Request,SNK,0,9,0,1,1')
        path.write_text(text)
        self.assertEqual(self.trace('--input', path)[-1]['phase'], 'HAVE_CAPS')

    def test_firmware_existing_regressions(self):
        exe = self.work / 'test_model.exe'
        subprocess.run([str(GCC), '-std=c99', '-Wall', '-Wextra', '-Werror',
                        '-I' + str(MODEL), str(ROOT / 'tests/test_pd_replay.c'), '-o', str(exe)], check=True)
        result = subprocess.run([str(exe)], capture_output=True, text=True, check=True)
        self.assertIn('PASS:', result.stdout)


if __name__ == '__main__':
    unittest.main(argv=[sys.argv[0]], verbosity=2)
