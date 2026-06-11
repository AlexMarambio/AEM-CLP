#!/usr/bin/env python3
import argparse
import subprocess
import sys
from pathlib import Path
from datetime import datetime


def parse_args():
    parser = argparse.ArgumentParser(
        description="Run BSG_CLP over a range of benchmarks and instances, saving raw logs."
    )
    parser.add_argument("bench_dir", help="Benchmark directory (e.g. problems/clp/benchs/BR)")
    parser.add_argument("--bench-range", dest="bench_range", required=True, help="e.g. 0-15")
    parser.add_argument("--instances",   dest="instances",   required=True, help="e.g. 1-100")
    parser.add_argument("--bsg-bin",     dest="bsg_bin",     default="./build/BSG_CLP")
    parser.add_argument("--prefix",      dest="prefix",      default="BR")
    parser.add_argument("--output-txt",  dest="output_txt",  default=None)
    parser.add_argument("-t", "--timelimit", dest="timelimit", type=int, default=None)
    parser.add_argument("-f", "--format",    dest="fmt",       default=None)
    return parser.parse_args()


def parse_range(s):
    a, b = [int(x) for x in s.split("-")]
    return a, b


def main():
    args = parse_args()

    bench_start, bench_end = parse_range(args.bench_range)
    inst_start,  inst_end  = parse_range(args.instances)

    bench_dir = Path(args.bench_dir)
    bench_files = []
    for i in range(bench_start, bench_end + 1):
        f = bench_dir / f"{args.prefix}{i}.txt"
        if not f.exists():
            print(f"Error: file not found: {f}")
            sys.exit(1)
        bench_files.append((i, f))

    txt_path = args.output_txt or f"bsg_log_{datetime.now().strftime('%Y%m%d_%H%M%S')}.txt"
    print(f"Logging to: {txt_path}")

    # Construir flags opcionales para BSG_CLP
    bsg_flags = []
    if args.timelimit is not None:
        bsg_flags += ["-t", str(args.timelimit)]
    if args.fmt is not None:
        bsg_flags += ["-f", args.fmt]

    with open(txt_path, "w", encoding="utf-8") as log:
        for bench_idx, bench_file in bench_files:
            for inst_id in range(inst_start, inst_end + 1):
                header = f"=== {args.prefix}{bench_idx}  instance {inst_id} ==="
                print(header)
                log.write(header + "\n")

                # Orden correcto: ejecutable, instance-set, flags
                cmd = [args.bsg_bin, str(bench_file), "-i", str(inst_id)] + bsg_flags
                result = subprocess.run(
                    cmd,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    text=True,
                )

                log.write(result.stdout)
                log.write("\n")

    print(f"\nDone. Log saved to: {txt_path}")


if __name__ == "__main__":
    main()