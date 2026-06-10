#!/usr/bin/env python3
import argparse
import os
import re
import subprocess
import sys
import time
from pathlib import Path

STAT_RE = re.compile(r"^\[STATS\] ([^=]+)=(.+)$")
MCTS_LINE_RE = re.compile(r"^mcts: (.*)$")
VALUE_LINE_RE = re.compile(r"^\s*([0-9]+(?:\.[0-9]+)?)\s*$")


def parse_args(argv):
    parser = argparse.ArgumentParser(
        description="Run BSG_CLP baseline and MCTS variants in parallel and compare results."
    )
    parser.add_argument(
        "bench_path",
        help="Benchmark file or benchmark directory (e.g. problems/clp/benchs/BR)"
    )
    parser.add_argument(
        "--range",
        dest="range",
        help="Range of BR files to run, in the form x-y (only when bench_path is a directory)",
        default=None,
    )
    parser.add_argument(
        "--bsg-bin",
        dest="bsg_bin",
        default="./build/BSG_CLP",
        help="Path to the BSG_CLP executable",
    )
    parser.add_argument(
        "--output-csv",
        dest="output_csv",
        help="Write comparison results to CSV file",
        default=None,
    )
    parser.add_argument(
        "--prefix",
        dest="prefix",
        default="BR",
        help="Prefix for benchmark files when using --range, e.g. BR",
    )
    parser.add_argument("--verbose", action="store_true", help="Show full process output")
    return parser.parse_args(argv)


def expand_benchmarks(bench_path, rng, prefix):
    path = Path(bench_path)
    if path.is_dir():
        if not rng:
            raise ValueError("When bench_path is a directory, --range x-y is required")
        try:
            start, end = [int(x) for x in rng.split("-")]
        except Exception:
            raise ValueError("--range must be in the form x-y")
        if start > end:
            raise ValueError("--range must satisfy x <= y")
        files = []
        for i in range(start, end + 1):
            candidate = path / f"{prefix}{i}.txt"
            if not candidate.exists():
                raise FileNotFoundError(f"Benchmark file not found: {candidate}")
            files.append(candidate)
        return files
    if path.is_file():
        return [path]
    raise FileNotFoundError(f"Benchmark path not found: {bench_path}")


def parse_stats(output):
    stats = {}
    lines = output.strip().splitlines()
    for line in lines:
        m = STAT_RE.match(line)
        if m:
            key = m.group(1).strip()
            value = m.group(2).strip()
            try:
                if "." in value:
                    stats[key] = float(value)
                else:
                    stats[key] = int(value)
            except ValueError:
                stats[key] = value
    return stats


def extract_final_value(output):
    for line in reversed(output.strip().splitlines()):
        m = VALUE_LINE_RE.match(line)
        if m:
            return float(m.group(1))
    return None


def start_process(label, bsg_bin, bench_file, bsg_args, mcts_enabled):
    args = [bsg_bin, str(bench_file)] + bsg_args
    if mcts_enabled:
        args.append("--mcts")
    process = subprocess.Popen(
        args,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    return {
        "label": label,
        "process": process,
        "start_time": time.perf_counter(),
        "mcts_enabled": 1 if mcts_enabled else 0,
        "bench_file": str(bench_file),
    }


def collect_process(proc_info):
    stdout, _ = proc_info["process"].communicate()
    elapsed = time.perf_counter() - proc_info["start_time"]
    stats = parse_stats(stdout)
    if "final_value" not in stats:
        fallback = extract_final_value(stdout)
        if fallback is not None:
            stats["final_value"] = fallback
    stats["process_time_s"] = elapsed
    stats["exit_code"] = proc_info["process"].returncode
    stats["mcts_enabled"] = proc_info["mcts_enabled"]
    stats["label"] = proc_info["label"]
    stats["bench_file"] = proc_info["bench_file"]
    stats["stdout"] = stdout
    return stats


def run_pair(bsg_bin, bench_file, bsg_args):
    baseline_info = start_process("BSS", bsg_bin, bench_file, bsg_args, False)
    mcts_info = start_process("BSS+MCTS", bsg_bin, bench_file, bsg_args, True)
    baseline_stats = collect_process(baseline_info)
    mcts_stats = collect_process(mcts_info)
    return baseline_stats, mcts_stats


def format_value(value):
    if value is None:
        return "-"
    if isinstance(value, (int, float)):
        return f"{value:.2f}"
    return str(value)


def format_row(name, base, mcts):
    base_str = format_value(base)
    mcts_str = format_value(mcts)
    delta = "-"
    if isinstance(base, (int, float)) and isinstance(mcts, (int, float)):
        delta = f"{(mcts - base):+.2f}"
    return f"{name:30} | {base_str:>10} | {mcts_str:>10} | {delta:>10}"


def print_comparison(baseline, mcts, bench_file, verbose=False):
    print("\n=== Comparison for {} ===".format(bench_file))
    if verbose:
        print("--- baseline stdout ---")
        print(baseline["stdout"])
        print("--- mcts stdout ---")
        print(mcts["stdout"])
    print("\nMetric                        |   BSS     |    MCTS   |    Delta")
    print("------------------------------+-----------+-----------+-----------")
    print(format_row("Final value (%)", baseline.get("final_value"), mcts.get("final_value")))
    print(format_row("Wall time (s)", baseline.get("wall_time_s", baseline.get("process_time_s")), mcts.get("wall_time_s", mcts.get("process_time_s"))))
    print(format_row("Process elapsed (s)", baseline.get("process_time_s"), mcts.get("process_time_s")))
    print(format_row("MCTS enabled", baseline.get("mcts_enabled"), mcts.get("mcts_enabled")))
    print(format_row("MCTS iter", baseline.get("mcts_iter"), mcts.get("mcts_iter")))
    print(format_row("MCTS depth", baseline.get("mcts_depth"), mcts.get("mcts_depth")))
    print(format_row("MCTS width", baseline.get("mcts_width"), mcts.get("mcts_width")))
    print(format_row("MCTS c", baseline.get("mcts_c"), mcts.get("mcts_c")))
    print(format_row("MCTS top", baseline.get("mcts_top"), mcts.get("mcts_top")))
    print(format_row("MCTS vcs_weight", baseline.get("mcts_vcs_weight"), mcts.get("mcts_vcs_weight")))
    print(format_row("MCTS mcts_weight", baseline.get("mcts_mcts_weight"), mcts.get("mcts_mcts_weight")))
    print(format_row("MCTS greedy complete", baseline.get("mcts_complete_with_greedy"), mcts.get("mcts_complete_with_greedy")))

    if baseline.get("exit_code", 0) != 0 or mcts.get("exit_code", 0) != 0:
        print("\nWarning: One of the runs exited with non-zero status")


def main():
    if "--" in sys.argv:
        idx = sys.argv.index("--")
        wrapper_args = sys.argv[1:idx]
        bsg_args = sys.argv[idx + 1:]
    else:
        wrapper_args = sys.argv[1:]
        bsg_args = []

    args = parse_args(wrapper_args)
    bench_files = expand_benchmarks(args.bench_path, args.range, args.prefix)
    bsg_args = [arg for arg in bsg_args if arg != "--mcts"]

    print(f"Found {len(bench_files)} benchmark file(s) to compare.")
    for bench_file in bench_files:
        baseline, mcts = run_pair(args.bsg_bin, bench_file, bsg_args)
        print_comparison(baseline, mcts, bench_file, args.verbose)
        if args.output_csv:
            write_csv(args.output_csv, baseline, mcts)


def write_csv(path, baseline, mcts):
    import csv
    exists = os.path.exists(path)
    with open(path, "a", newline="") as csvfile:
        writer = csv.writer(csvfile)
        if not exists:
            writer.writerow([
                "bench_file",
                "mode",
                "final_value",
                "wall_time_s",
                "process_time_s",
                "mcts_enabled",
                "mcts_iter",
                "mcts_depth",
                "mcts_width",
                "mcts_c",
                "mcts_top",
                "mcts_vcs_weight",
                "mcts_mcts_weight",
                "mcts_complete_with_greedy",
                "exit_code",
            ])
        for stats in (baseline, mcts):
            writer.writerow([
                stats.get("bench_file"),
                stats.get("label"),
                stats.get("final_value"),
                stats.get("wall_time_s"),
                stats.get("process_time_s"),
                stats.get("mcts_enabled"),
                stats.get("mcts_iter"),
                stats.get("mcts_depth"),
                stats.get("mcts_width"),
                stats.get("mcts_c"),
                stats.get("mcts_top"),
                stats.get("mcts_vcs_weight"),
                stats.get("mcts_mcts_weight"),
                stats.get("mcts_complete_with_greedy"),
                stats.get("exit_code"),
            ])


if __name__ == "__main__":
    main()
