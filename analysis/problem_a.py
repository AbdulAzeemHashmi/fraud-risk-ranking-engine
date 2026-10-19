"""Reproducible Problem A profile. Python is used here for pandas/NumPy analysis and plotting."""

from __future__ import annotations

import argparse
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd


ROLL_NUMBER = 2013
FEATURES = [f"V{i}" for i in range(1, 29)]
REQUIRED = ["Time", *FEATURES, "Amount", "Class"]


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("csv", type=Path, help="Path to Kaggle creditcard.csv")
    parser.add_argument("--out", type=Path, default=Path("results/problem_a"))
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)

    data = pd.read_csv(args.csv)
    if list(data.columns) != REQUIRED:
        raise ValueError(f"Expected Kaggle columns Time,V1..V28,Amount,Class; got {list(data.columns)}")
    rows_before = len(data)
    missing = int(data.isna().sum().sum())
    non_numeric = data[REQUIRED].apply(pd.to_numeric, errors="coerce").isna().sum().sum()
    non_finite = int((~np.isfinite(data[REQUIRED].to_numpy(dtype=np.float64))).sum())
    if missing or non_numeric or non_finite:
        raise ValueError(
            f"Input contains missing={missing}, non_numeric={non_numeric}, non_finite={non_finite}; "
            "no rows were silently dropped or imputed."
        )

    # txn_id is the one-based physical data-row number, matching the assignment definition.
    data.insert(0, "txn_id", np.arange(1, rows_before + 1, dtype=np.int64))
    mu = data[FEATURES].to_numpy(dtype=np.float64).mean(axis=0)
    sigma = data[FEATURES].to_numpy(dtype=np.float64).std(axis=0, ddof=0)
    sigma[sigma == 0] = 1.0
    z = (data[FEATURES].to_numpy(dtype=np.float64) - mu) / sigma
    risk = np.einsum("ij,ij->i", z, z)
    bands = np.floor(np.log2(np.maximum(risk, np.finfo(np.float64).tiny))).astype(np.int8)
    data["band"] = np.clip(bands, 0, 9)

    band_counts = data["band"].value_counts().reindex(range(10), fill_value=0).sort_index()
    fraud_counts = data.loc[data["Class"].eq(1), "band"].value_counts().reindex(range(10), fill_value=0).sort_index()
    amount_freq = data["Amount"].value_counts()
    fraud_total = int(data["Class"].eq(1).sum())
    profile = pd.DataFrame({
        "band": range(10),
        "transactions": band_counts.to_numpy(),
        "transaction_share": band_counts.to_numpy() / rows_before,
        "frauds": fraud_counts.to_numpy(),
        "fraud_share": fraud_counts.to_numpy() / max(fraud_total, 1),
    })
    profile.to_csv(args.out / "risk_band_distribution.csv", index=False)

    fig, ax = plt.subplots(figsize=(8.5, 4.8), constrained_layout=True)
    ax.bar(profile["band"], profile["transactions"], color="#2864a5", label="Transactions")
    ax.set_xlabel("Risk band (unitless, 0–9)")
    ax.set_ylabel("Transactions (count)")
    ax.set_xticks(range(10))
    ax.set_title("Problem A — Distribution of risk bands")
    ax.legend()
    fig.text(0.01, -0.025, "Figure A1. Full-file risk-band distribution; bands use population standard deviation (ddof=0).")
    fig.savefig(args.out / "risk_band_histogram.png", dpi=180, bbox_inches="tight")
    plt.close(fig)

    # Triage precedence: band desc, Amount desc, Time asc, txn_id asc.
    triage_order = np.lexsort((data["txn_id"].to_numpy(), data["Time"].to_numpy(),
                               -data["Amount"].to_numpy(), -data["band"].to_numpy()))
    raw_order = np.arange(rows_before)
    is_triage_ranked = bool(np.array_equal(triage_order, raw_order))
    is_reverse_ranked = bool(np.array_equal(triage_order[::-1], raw_order))
    rank_at_raw_position = np.empty(rows_before, dtype=np.int64)
    rank_at_raw_position[triage_order] = np.arange(rows_before)
    same_position_share = float(np.mean(rank_at_raw_position == raw_order))
    out_of_position = int(np.count_nonzero(rank_at_raw_position != raw_order))
    top10_count = int(amount_freq.head(10).sum())

    summary = {
        "rows": rows_before,
        "frauds": fraud_total,
        "missing_cells": missing,
        "non_numeric_cells": int(non_numeric),
        "non_finite_cells": non_finite,
        "duplicate_full_rows": int(data[REQUIRED].duplicated().sum()),
        "distinct_amount_values": int(data["Amount"].nunique()),
        "top_10_amount_value_share": top10_count / rows_before,
        "amount_top_10_values_and_counts": amount_freq.head(10).to_dict(),
        "roll_number": ROLL_NUMBER,
        "K": 25 + ROLL_NUMBER % 226,
        "std_convention": "population standard deviation (NumPy ddof=0)",
        "raw_time_nondecreasing_C1": bool(data["Time"].is_monotonic_increasing),
        "raw_exact_triage_rank_C3": is_triage_ranked,
        "raw_exact_reverse_triage_rank_C4": is_reverse_ranked,
        "raw_records_out_of_triage_position": out_of_position,
        "raw_fraction_in_same_triage_position": same_position_share,
        "C2_to_C6": "constructed conditions; not intrinsic raw-file properties",
        "raw_condition_evidence": (
            f"Time is nondecreasing={data['Time'].is_monotonic_increasing}; "
            f"raw equals triage order={is_triage_ranked}; raw equals its exact reverse={is_reverse_ranked}. "
            "C2 (seeded shuffle), C4 (exact reverse), C5 (1% displacement), and C6 (band-only key) "
            "are experiment constructions from the same records."
        ),
    }
    pd.Series(summary, dtype=object).to_json(args.out / "summary.json", indent=2)

    print(f"rows={rows_before}; frauds={fraud_total}; roll_number={ROLL_NUMBER}; K={summary['K']}")
    print(f"cleaning: missing={missing}, non_numeric={non_numeric}, non_finite={non_finite}, "
          f"duplicate_full_rows={summary['duplicate_full_rows']}; no rows dropped or imputed")
    print(f"distinct Amount={summary['distinct_amount_values']}; top-10 Amount values cover "
          f"{top10_count}/{rows_before} = {summary['top_10_amount_value_share']:.6%} of rows")
    print(profile.to_string(index=False))
    print(summary["raw_condition_evidence"])
    print(f"Saved table, summary, and histogram under {args.out}")


if __name__ == "__main__":
    main()
