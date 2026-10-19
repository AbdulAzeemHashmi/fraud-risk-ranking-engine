"""Create the compact C, D, and E tables from the experiment runner's results.csv."""

from __future__ import annotations

import argparse
import csv
from collections import defaultdict
from pathlib import Path
from statistics import median


def write_csv(path: Path, fields: list[str], rows: list[dict[str, str]]) -> None:
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("results_csv", type=Path, help="Experiment runner results.csv")
    parser.add_argument("--out", type=Path, help="Output folder; defaults to the CSV folder")
    args = parser.parse_args()
    out_dir = args.out or args.results_csv.parent
    out_dir.mkdir(parents=True, exist_ok=True)

    with args.results_csv.open(newline="", encoding="utf-8-sig") as stream:
        records = list(csv.DictReader(stream))

    c_fields = ["condition", "n", "algorithm", "seconds", "comparisons",
                "moves", "extra_memory_bytes", "stable", "status"]
    c_rows = [
        {("median_seconds" if key == "seconds" else key): row[key] for key in c_fields}
        for row in records if row["problem"] == "C" and row["status"] == "median"
    ]
    c_fields[3] = "median_seconds"
    write_csv(out_dir / "C_summary.csv", c_fields, c_rows)

    d_fields = ["condition", "n", "k", "algorithm", "seconds", "comparisons",
                "moves", "extra_memory_bytes", "stable", "captured_frauds", "status"]
    d_rows = [
        {("median_seconds" if key == "seconds" else key): row[key] for key in d_fields}
        for row in records if row["problem"] == "D" and row["status"] == "median"
    ]
    d_fields[4] = "median_seconds"
    write_csv(out_dir / "D_summary.csv", d_fields, d_rows)

    e_groups: dict[tuple[str, str], list[dict[str, str]]] = defaultdict(list)
    for row in records:
        if row["problem"] == "E" and row["status"] == "ok":
            e_groups[(row["condition"], row["algorithm"])].append(row)
    e_fields = ["condition", "window_count", "algorithm", "median_final_cumulative_seconds",
                "peak_extra_memory_bytes"]
    e_rows: list[dict[str, str]] = []
    for (condition, algorithm), rows in sorted(e_groups.items()):
        last_window = max(int(row["window_index"]) for row in rows)
        finals = [row for row in rows if int(row["window_index"]) == last_window]
        final_times = [float(row["cumulative_seconds"]) for row in finals]
        peak = max(int(row["peak_extra_memory_bytes"]) for row in rows)
        e_rows.append({
            "condition": condition,
            "window_count": str(last_window),
            "algorithm": algorithm,
            "median_final_cumulative_seconds": f"{median(final_times):.10g}",
            "peak_extra_memory_bytes": str(peak),
        })
    write_csv(out_dir / "E_summary.csv", e_fields, e_rows)
    print(f"C summary rows={len(c_rows)}; D summary rows={len(d_rows)}; E summary rows={len(e_rows)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
