#!/usr/bin/env python3
import argparse
import os
import re
import subprocess
import sys
import time
from pathlib import Path
from datetime import datetime

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
        "--instances",
        dest="instances",
        help="Range of -i instances to run per benchmark, in the form x-y (e.g. 15-47). "
             "Overrides any -i passed after --.",
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
        "--output-txt",
        dest="output_txt",
        help="Write comparison results to TXT file (default: results_<timestamp>.txt)",
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


def parse_instance_range(instances_str):
    """Parsea '15-47' y retorna (15, 47). Retorna None si es None."""
    if instances_str is None:
        return None
    try:
        start, end = [int(x) for x in instances_str.split("-")]
    except Exception:
        raise ValueError("--instances must be in the form x-y, e.g. 15-47")
    if start > end:
        raise ValueError("--instances must satisfy x <= y")
    return start, end


def strip_i_flag(bsg_args):
    """Elimina cualquier -i <val> o -i<val> de bsg_args para que --instances tome precedencia."""
    cleaned = []
    skip_next = False
    for arg in bsg_args:
        if skip_next:
            skip_next = False
            continue
        if arg == "-i":
            skip_next = True   # el siguiente token es el valor, también lo saltamos
            continue
        if re.match(r"^-i\d+$", arg):
            continue            # forma compacta: -i47
        cleaned.append(arg)
    return cleaned


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
    """Corre un par BSS / BSS+MCTS para los bsg_args dados (ya incluyen -i si aplica)."""
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


def build_comparison_lines(baseline, mcts, bench_file, instance_id=None, verbose=False):
    """Genera las líneas de comparación como lista de strings."""
    header = str(bench_file)
    if instance_id is not None:
        header += f"  instance {instance_id}"
    lines = []
    lines.append("\n=== Comparison for {} ===".format(header))
    if verbose:
        lines.append("--- baseline stdout ---")
        lines.append(baseline["stdout"])
        lines.append("--- mcts stdout ---")
        lines.append(mcts["stdout"])
    lines.append("\nMetric                        |   BSS     |    MCTS   |    Delta")
    lines.append("------------------------------+-----------+-----------+-----------")
    lines.append(format_row("Final value (%)", baseline.get("final_value"), mcts.get("final_value")))
    lines.append(format_row("Wall time (s)", baseline.get("wall_time_s", baseline.get("process_time_s")), mcts.get("wall_time_s", mcts.get("process_time_s"))))
    lines.append(format_row("Process elapsed (s)", baseline.get("process_time_s"), mcts.get("process_time_s")))
    lines.append(format_row("MCTS enabled", baseline.get("mcts_enabled"), mcts.get("mcts_enabled")))
    lines.append(format_row("MCTS iter", baseline.get("mcts_iter"), mcts.get("mcts_iter")))
    lines.append(format_row("MCTS depth", baseline.get("mcts_depth"), mcts.get("mcts_depth")))
    lines.append(format_row("MCTS width", baseline.get("mcts_width"), mcts.get("mcts_width")))
    lines.append(format_row("MCTS c", baseline.get("mcts_c"), mcts.get("mcts_c")))
    lines.append(format_row("MCTS top", baseline.get("mcts_top"), mcts.get("mcts_top")))
    lines.append(format_row("MCTS vcs_weight", baseline.get("mcts_vcs_weight"), mcts.get("mcts_vcs_weight")))
    lines.append(format_row("MCTS mcts_weight", baseline.get("mcts_mcts_weight"), mcts.get("mcts_mcts_weight")))
    lines.append(format_row("MCTS greedy complete", baseline.get("mcts_complete_with_greedy"), mcts.get("mcts_complete_with_greedy")))

    if baseline.get("exit_code", 0) != 0 or mcts.get("exit_code", 0) != 0:
        lines.append("\nWarning: One of the runs exited with non-zero status")

    return lines


def build_summary_lines(summaries, bench_file):
    """Genera un bloque de resumen promedio para un benchmark dado."""
    if not summaries:
        return []

    def avg(key, mode):
        vals = [s[mode].get(key) for s in summaries if isinstance(s[mode].get(key), (int, float))]
        return sum(vals) / len(vals) if vals else None

    lines = []
    lines.append(f"\n{'='*60}")
    lines.append(f"SUMMARY for {bench_file}  ({len(summaries)} instances)")
    lines.append(f"{'='*60}")
    lines.append("\nMetric                        |   BSS     |    MCTS   |    Delta")
    lines.append("------------------------------+-----------+-----------+-----------")
    lines.append(format_row("Avg final value (%)", avg("final_value", "baseline"), avg("final_value", "mcts")))
    lines.append(format_row("Avg process time (s)", avg("process_time_s", "baseline"), avg("process_time_s", "mcts")))
    return lines


def print_comparison(baseline, mcts, bench_file, instance_id=None, verbose=False):
    lines = build_comparison_lines(baseline, mcts, bench_file, instance_id, verbose)
    for line in lines:
        print(line)


def append_txt(path, lines):
    """Agrega líneas al archivo .txt."""
    with open(path, "a", encoding="utf-8") as f:
        for line in lines:
            f.write(line + "\n")


def init_txt(path):
    """Escribe el encabezado del archivo .txt con timestamp."""
    with open(path, "w", encoding="utf-8") as f:
        f.write("BSG vs BSG+MCTS Comparison Results\n")
        f.write(f"Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        f.write("=" * 60 + "\n")


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

    # Parsear rango de instancias
    instance_range = parse_instance_range(args.instances)
    if instance_range is not None:
        # Eliminar -i que venga en bsg_args para que no colisione
        bsg_args = strip_i_flag(bsg_args)
        inst_start, inst_end = instance_range
        instance_ids = list(range(inst_start, inst_end + 1))
    else:
        # Sin --instances: usar los args tal como vienen (puede tener -i fijo o ninguno)
        instance_ids = None

    # Determinar path del .txt
    txt_path = args.output_txt
    if txt_path is None:
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        txt_path = f"results_{timestamp}.txt"

    init_txt(txt_path)
    print(f"Saving TXT output to: {txt_path}")

    if instance_ids is not None:
        print(f"Found {len(bench_files)} benchmark file(s), "
              f"{len(instance_ids)} instance(s) each "
              f"(i={inst_start}..{inst_end}).")
    else:
        print(f"Found {len(bench_files)} benchmark file(s) to compare.")

    for bench_file in bench_files:
        summaries = []  # acumula resultados de todas las instancias de este benchmark

        if instance_ids is not None:
            print(f"\n>>> Benchmark: {bench_file}")
            for inst_id in instance_ids:
                run_args = bsg_args + ["-i", str(inst_id)]
                print(f"    instance {inst_id}...", end=" ", flush=True)
                baseline, mcts = run_pair(args.bsg_bin, bench_file, run_args)
                print(f"BSS={format_value(baseline.get('final_value'))}  "
                      f"MCTS={format_value(mcts.get('final_value'))}")

                cmp_lines = build_comparison_lines(baseline, mcts, bench_file, inst_id, args.verbose)
                for line in cmp_lines:
                    print(line) if args.verbose else None
                append_txt(txt_path, cmp_lines)

                summaries.append({"baseline": baseline, "mcts": mcts})

                if args.output_csv:
                    write_csv(args.output_csv, baseline, mcts, inst_id)

            # Resumen promedio del benchmark
            summary_lines = build_summary_lines(summaries, bench_file)
            for line in summary_lines:
                print(line)
            append_txt(txt_path, summary_lines)

        else:
            # Comportamiento original: un par por benchmark sin iterar instancias
            baseline, mcts = run_pair(args.bsg_bin, bench_file, bsg_args)
            cmp_lines = build_comparison_lines(baseline, mcts, bench_file, verbose=args.verbose)
            print_comparison(baseline, mcts, bench_file, verbose=args.verbose)
            append_txt(txt_path, cmp_lines)
            if args.output_csv:
                write_csv(args.output_csv, baseline, mcts)

    print(f"\nResults saved to: {txt_path}")


def write_csv(path, baseline, mcts, instance_id=None):
    import csv
    exists = os.path.exists(path)
    with open(path, "a", newline="") as csvfile:
        writer = csv.writer(csvfile)
        if not exists:
            writer.writerow([
                "bench_file",
                "instance_id",
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
                instance_id,
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