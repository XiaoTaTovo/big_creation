"""Build the PC tool against the actual firmware model; never program hardware."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent
DEFAULT_MODEL = Path('D:/01_Workspace/12_Dev_Projects/AllCode/QinHen/ST77789_LVGL/ST7789_TEST/V5F/User')
DEFAULT_GCC = Path('D:/software/Code/MinGW/mingw64/bin/gcc.exe')


def build(model=DEFAULT_MODEL, gcc=DEFAULT_GCC):
    header = model / 'pd_replay_model.h'
    if not header.is_file() or not gcc.is_file():
        raise RuntimeError('Model header or GCC missing. Supply --model-dir and --gcc explicitly.')
    if (HERE / 'pd_replay_model.h').exists():
        raise RuntimeError('A local model copy shadows the firmware model; remove the duplicate manually.')
    output = HERE / 'build'
    output.mkdir(exist_ok=True)
    exe = output / 'replay_cli.exe'
    subprocess.run([str(gcc), '-std=c11', '-Wall', '-Wextra', '-Werror',
                    '-I' + str(model), str(HERE / 'replay_cli.c'), '-o', str(exe)], check=True)
    # Record what was actually included, not just the -I search directory.
    deps = subprocess.check_output([str(gcc), '-MM', '-I' + str(model),
                                    str(HERE / 'replay_cli.c')], text=True, encoding='utf-8')
    (output / 'dependencies.txt').write_text(deps, encoding='utf-8')
    (output / 'model-source.json').write_text(json.dumps({
        'schema_version': 1, 'model_path': str(header.resolve()),
        'sha256': hashlib.sha256(header.read_bytes()).hexdigest(),
        'scope': 'CC1/SOP fixture; fixed PDO/no GiveBack subset; not a wire protocol'
    }, indent=2, ensure_ascii=False), encoding='utf-8')
    return exe


def main():
    parser = argparse.ArgumentParser(description=__doc__, add_help=False)
    parser.add_argument('--model-dir', type=Path, default=DEFAULT_MODEL)
    parser.add_argument('--gcc', type=Path, default=DEFAULT_GCC)
    parser.add_argument('--test', action='store_true')
    args, remaining = parser.parse_known_args()
    try:
        exe = build(args.model_dir, args.gcc)
        if args.test:
            return subprocess.call([sys.executable, str(HERE / 'test_replay.py'),
                                    str(exe), str(args.model_dir), str(args.gcc)])
        return subprocess.call([str(exe), *remaining])
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f'ERROR: {error}', file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
