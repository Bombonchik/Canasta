#!/usr/bin/env python3
import os
import sys
import argparse

def count_lines(root):
    cpp_lines = 0
    hpp_lines = 0

    for dirpath, _, filenames in os.walk(root):
        for fname in filenames:
            if not fname.endswith(('.cpp', '.hpp')):
                continue

            path = os.path.join(dirpath, fname)
            try:
                with open(path, 'r', encoding='utf-8', errors='ignore') as f:
                    lines = sum(1 for _ in f)
            except Exception as e:
                print(f"Warning: could not read {path!r}: {e}", file=sys.stderr)
                continue

            # print per-file
            print(f"{path}: {lines} lines")

            if fname.endswith('.cpp'):
                cpp_lines += lines
            else:
                hpp_lines += lines

    return cpp_lines, hpp_lines

def main():
    parser = argparse.ArgumentParser(
        description="Count lines in .cpp and .hpp files under given dirs (default: current dir),\n"
                    "and print each filename + its count."
    )
    parser.add_argument(
        'dirs',
        nargs='*',
        default=['.'],
        help='One or more directories to scan'
    )
    args = parser.parse_args()

    total_cpp = 0
    total_hpp = 0

    for d in args.dirs:
        if not os.path.isdir(d):
            print(f"Error: {d!r} is not a directory", file=sys.stderr)
            continue

        print(f"\n== Scanning {d!r} ==")
        cpp, hpp = count_lines(d)
        print(f"\nSummary for {d!r}: .cpp={cpp}  .hpp={hpp}  sum={cpp + hpp}")
        total_cpp += cpp
        total_hpp += hpp

    grand_total = total_cpp + total_hpp
    if len(args.dirs) > 1:
        print("\n" + ("=" * 40))
        print(f"TOTAL across {len(args.dirs)} dirs: .cpp={total_cpp}  .hpp={total_hpp}  sum={grand_total}")

if __name__ == '__main__':
    main()