"""Host reducer tests + fresh WCH compilation/link, without make or hardware access.

Read source lists and compiler flags from MRS-generated makefiles. All build
artifacts go into a separate directory; original obj outputs are not changed.
"""
import argparse
import concurrent.futures
from pathlib import Path
import re
import shlex
import subprocess


def run(args, cwd=None):
    result = subprocess.run([str(x) for x in args], cwd=cwd, capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    return result.stdout + result.stderr


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--project', type=Path, required=True, help='ST7789_TEST root')
    parser.add_argument('--wch-bin', type=Path, required=True)
    parser.add_argument('--host-gcc', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    repo = Path(__file__).resolve().parents[1]
    user = args.project / 'V5F/User'
    print(run([args.host_gcc, '-std=c99', '-Wall', '-Wextra', '-Werror',
               '-I' + str(user), repo / 'tests/test_pd_replay.c', '-o', output / 'test_replay.exe']))
    print(run([output / 'test_replay.exe']), flush=True)
    obj = args.project / 'V5F/obj'
    makefile = (obj / 'makefile').read_text()
    def flags(name):
        raw = re.search(r'^' + name + r'\s*:?=\s*(.+)$', makefile, re.M)[1]
        return shlex.split(raw.replace('\\', '/'))
    cflags, aflags, ldflags = flags('FLAGS_C'), flags('FLAGS_ASM'), flags('FLAGS_LD')
    gcc = args.wch_bin / 'riscv-wch-elf-gcc.exe'
    sources = []
    # Only active includes: obj may retain stale makefiles for excluded tests.
    subdirs = re.findall(r'^-include (.+/subdir\.mk)\s*$', makefile, re.M)
    for subdir in (obj / name.strip() for name in subdirs):
        content = subdir.read_text()
        for variable in ['C_SRCS', 'S_UPPER_SRCS']:
            match = re.search(r'^' + variable + r'\s*\+=\s*\\\s*\n(.*?)(?:\n\s*\n|\Z)', content, re.M | re.S)
            if match:
                for line in match[1].splitlines():
                    source = line.strip().rstrip('\\').strip()
                    if source:
                        sources.append((obj / source).resolve())
    assert sources and len(sources) == len(set(sources))
    def compile_one(item):
        i, source = item
        target = output / f'{i:04d}.o'
        log = run([gcc, *(aflags if source.suffix == '.S' else cflags),
                   '-c', source, '-o', target])
        return target, log
    with concurrent.futures.ThreadPoolExecutor(max_workers=8) as pool:
        results = list(pool.map(compile_one, enumerate(sources)))
    (output / 'compile.log').write_text(''.join(log for _, log in results), encoding='utf-8')
    response = output / 'objects.rsp'
    response.write_text('\n'.join('"' + p.as_posix() + '"' for p, _ in results), encoding='utf-8')
    elf = output / 'pd_replay.elf'
    print(run([gcc, *ldflags, '-Wl,--print-memory-usage', '-o', elf, '@' + str(response), '-lm'], cwd=output))
    print(run([args.wch_bin / 'riscv-wch-elf-size.exe', elf]))
    print(f'PASS: freshly compiled and linked {len(sources)} sources; outputs: {output}')


if __name__ == '__main__':
    main()
