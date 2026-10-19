"""Build the compact report PDF and refreshed C/D/E report from saved results."""

from __future__ import annotations

import csv
import json
import statistics
from collections import defaultdict
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER
from reportlab.lib.pagesizes import letter
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib.units import inch
from reportlab.platypus import (Image, PageBreak, Paragraph, SimpleDocTemplate,
                                Spacer, Table, TableStyle)


ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "results" / "experiments_final_v2"
PDF = ROOT / "2013-A2-A.pdf"
FIG = OUT / "report_figures"
FIG.mkdir(parents=True, exist_ok=True)


def rows(name: str) -> list[dict[str, str]]:
    with (OUT / name).open(newline="", encoding="utf-8-sig") as f:
        return list(csv.DictReader(f))


def table_data(data: list[list[object]], widths: list[float] | None = None,
               font: int = 7) -> Table:
    t = Table(data, colWidths=widths, repeatRows=1, hAlign="LEFT")
    t.setStyle(TableStyle([
        ("BACKGROUND", (0, 0), (-1, 0), colors.HexColor("#1f3b5b")),
        ("TEXTCOLOR", (0, 0), (-1, 0), colors.white),
        ("FONTNAME", (0, 0), (-1, 0), "Helvetica-Bold"),
        ("FONTNAME", (0, 1), (-1, -1), "Helvetica"),
        ("FONTSIZE", (0, 0), (-1, -1), font),
        ("LEADING", (0, 0), (-1, -1), font + 1.2),
        ("GRID", (0, 0), (-1, -1), 0.25, colors.HexColor("#b8c4d0")),
        ("ROWBACKGROUNDS", (0, 1), (-1, -1), [colors.white, colors.HexColor("#f1f5f8")]),
        ("VALIGN", (0, 0), (-1, -1), "MIDDLE"),
        ("LEFTPADDING", (0, 0), (-1, -1), 4),
        ("RIGHTPADDING", (0, 0), (-1, -1), 4),
        ("TOPPADDING", (0, 0), (-1, -1), 3),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 3),
    ]))
    return t


def pstyle(base, name, **kwargs):
    return ParagraphStyle(name, parent=base, **kwargs)


def build_figures(c: list[dict[str, str]], d: list[dict[str, str]], e: list[dict[str, str]],
                  bands: list[dict[str, str]]) -> dict[str, Path]:
    plt.rcParams.update({"font.size": 7, "axes.titlesize": 9, "axes.labelsize": 7,
                         "legend.fontsize": 6, "figure.dpi": 160})
    paths: dict[str, Path] = {}
    algs = ["Insertion Sort", "Merge Sort", "Naive Quick Sort", "Hardened Quick Sort", "Heap Sort"]
    labels = ["Insertion", "Merge", "Naive QS", "Hardened QS", "Heap"]
    sizes = [1000, 10000, 100000, 284807]
    c2 = {(int(r["n"]), r["algorithm"]): r for r in c if r["condition"] == "C2"}
    fig, ax = plt.subplots(figsize=(5.2, 2.35))
    for alg, lab in zip(algs, labels):
        ax.plot(sizes, [float(c2[(n, alg)]["median_seconds"]) for n in sizes], marker="o", label=lab)
    ax.set(xscale="log", yscale="log", xlabel="n", ylabel="Median seconds", title="C2: time by input size")
    ax.grid(True, which="both", alpha=.25); ax.legend(ncol=3); fig.tight_layout()
    paths["c_time"] = FIG / "c_time.png"; fig.savefig(paths["c_time"]); plt.close(fig)

    for metric, title, file in [("comparisons", "C2: comparisons by input size", "c_comparisons"),
                                ("moves", "C2: moves by input size", "c_moves")]:
        fig, ax = plt.subplots(figsize=(5.2, 2.1))
        for alg, lab in zip(algs, labels):
            ax.plot(sizes, [int(c2[(n, alg)][metric]) for n in sizes], marker="o", label=lab)
        ax.set(xscale="log", yscale="log", xlabel="n", ylabel=metric.title(), title=title)
        ax.grid(True, which="both", alpha=.25); ax.legend(ncol=3); fig.tight_layout()
        paths[file] = FIG / f"{file}.png"; fig.savefig(paths[file]); plt.close(fig)

    fig, ax = plt.subplots(figsize=(5.2, 2.1))
    for alg, lab in zip(algs, labels):
        vals = [float(c2[(n, alg)]["median_seconds"]) / (n * __import__("math").log2(n)) for n in sizes]
        ax.plot(sizes, vals, marker="o", label=lab)
    ax.set(xscale="log", yscale="log", xlabel="n", ylabel="seconds / n log2(n)", title="C2: time normalized by n log n")
    ax.grid(True, which="both", alpha=.25); ax.legend(ncol=3); fig.tight_layout()
    paths["c_nlogn"] = FIG / "c_nlogn.png"; fig.savefig(paths["c_nlogn"]); plt.close(fig)
    fig, ax = plt.subplots(figsize=(5.2, 2.1))
    for alg, lab in zip(algs, labels):
        vals = [float(c2[(n, alg)]["median_seconds"]) / (n*n) for n in sizes]
        ax.plot(sizes, vals, marker="o", label=lab)
    ax.set(xscale="log", yscale="log", xlabel="n", ylabel="seconds / n squared", title="C2: time normalized by n squared")
    ax.grid(True, which="both", alpha=.25); ax.legend(ncol=3); fig.tight_layout()
    paths["c_n2"] = FIG / "c_n2.png"; fig.savefig(paths["c_n2"]); plt.close(fig)

    fig, ax = plt.subplots(figsize=(5.2, 2.1))
    conds = [f"C{i}" for i in range(1, 7)]; x = list(range(6)); width = .15
    for j, (alg, lab) in enumerate(zip(algs, labels)):
        vals = [float(next(r["median_seconds"] for r in c if r["n"] == "10000" and r["condition"] == cond and r["algorithm"] == alg)) for cond in conds]
        ax.bar([a + (j-2)*width for a in x], vals, width, label=lab)
    ax.set_xticks(x, conds); ax.set_ylabel("Median seconds"); ax.set_title("n = 10,000 across input conditions")
    ax.legend(ncol=3); fig.tight_layout()
    paths["c_conditions"] = FIG / "c_conditions.png"; fig.savefig(paths["c_conditions"]); plt.close(fig)

    fraud_curve = OUT / "fraud_capture_curve.csv"
    fc = list(csv.DictReader(fraud_curve.open(newline="", encoding="utf-8-sig")))
    fig, ax = plt.subplots(figsize=(5.1, 1.6))
    ax.plot([int(x["K"]) for x in fc], [float(x["fraction_of_all_frauds"]) for x in fc], color="#197a63")
    ax.axhline(.5, color="#aa3344", linestyle="--", linewidth=.8)
    ax.axvline(2068, color="#555555", linestyle=":", linewidth=.8)
    ax.set(xlabel="K", ylabel="Frauds captured", title="Fraud capture using the triage order")
    ax.grid(alpha=.2); fig.tight_layout()
    paths["d_capture"] = FIG / "d_capture.png"; fig.savefig(paths["d_capture"]); plt.close(fig)

    e_groups: dict[tuple[str, str], list[dict[str, str]]] = defaultdict(list)
    for r in e:
        e_groups[(r["condition"], r["algorithm"])].append(r)
    for cond, key, title in [("1-hour", "1hour", "Live feed: one hour windows"),
                             ("10-minute", "10min", "Live feed: ten minute windows")]:
        fig, ax = plt.subplots(figsize=(5.1, 1.6))
        for alg in ["Re-rank All with Merge Sort", "Maintain Size-K Min-Heap", "Merge Sorted Window into Top-K"]:
            trials: dict[int, list[float]] = defaultdict(list)
            for r in e_groups[(cond, alg)]:
                trials[int(r["window_index"])].append(float(r["cumulative_seconds"]))
            xi = sorted(trials); yi = [statistics.median(trials[i]) for i in xi]
            ax.plot(xi, yi, label=alg.replace(" with Merge Sort", "").replace("Maintain Size-K ", ""))
        ax.set(xlabel="Window number", ylabel="Cumulative seconds", title=title)
        ax.grid(alpha=.2); ax.legend(fontsize=5); fig.tight_layout()
        paths[f"e_{key}"] = FIG / f"e_{key}.png"; fig.savefig(paths[f"e_{key}"]); plt.close(fig)
    return paths


def main() -> None:
    c = rows("C_summary.csv")
    d = rows("D_summary.csv")
    e = rows("E_summary.csv")
    raw = rows("results.csv")
    bands = rows("..\\problem_a\\risk_band_distribution.csv") if False else []
    with (ROOT / "results" / "problem_a" / "risk_band_distribution.csv").open(newline="", encoding="utf-8-sig") as f:
        bands = list(csv.DictReader(f))
    pred = rows("prediction_log.csv")
    outcomes = [r for r in pred if r["phase"] == "result"]
    correct_w = sum(r["winner_prediction_correct"] == "true" for r in outcomes)
    correct_l = sum(r["loser_prediction_correct"] == "true" for r in outcomes)
    fig = build_figures(c, d, raw, bands)
    sizes = [1000, 10000, 100000, 284807]
    algs = ["Insertion Sort", "Merge Sort", "Naive Quick Sort", "Hardened Quick Sort", "Heap Sort"]

    # Markdown report with every required output location and refreshed values.
    report = OUT / "REPORT.md"
    c2 = {(int(r["n"]), r["algorithm"]): r for r in c if r["condition"] == "C2"}
    dcost = [r for r in d if r["n"] == "284807" and r["k"] == "2068"]
    txt = ["# Experiment results: Problems C, D, and E", "",
           "## Run setup", "",
           "All experiments use roll number and random seed 2013. Personal K is 230. The complete dataset has 284,807 rows and 492 frauds. Runs used C++17, MinGW GCC 16.2.0, `-O2 -Wall -Wextra -Wpedantic`, five trials, and the default 300 second per-trial timeout for C and D. C produced 120 median rows with no DNF rows. D produced 78 median rows with no DNF rows. E used five trials at each window size. Dataset reading and condition construction are outside the timers. `Class` is used only to evaluate queue capture.", "",
           "The runner checks full ordering and row preservation after every C run. C1 to C5 also match a `std::sort` reference; C6 checks band order and row preservation because equal bands may have any internal order. Comparisons count comparator calls. Moves count record-slot assignments. Memory values are algorithm storage estimates, not process RAM.", "",
           "## Problem C: full ranking tournament", "",
           f"There are 24 prediction rounds. The predicted winner was correct in {correct_w}/24 rounds and the predicted loser in {correct_l}/24 rounds. The full per-run prediction and result log is `prediction_log.csv`.", "",
           "Median seconds on C2, the seeded shuffle:", "",
           "| n | Insertion | Merge | Naive quicksort | Hardened quicksort | Heap |",
           "|---:|---:|---:|---:|---:|---:|"]
    for n in (1000, 10000, 100000, 284807):
        vals = [f"{float(c2[(n,a)]['median_seconds']):.6g}" for a in ["Insertion Sort","Merge Sort","Naive Quick Sort","Hardened Quick Sort","Heap Sort"]]
        txt.append(f"| {n:,} | " + " | ".join(vals) + " |")
    txt += ["", "Naive Quick Sort had the lowest C2 median at n = 100,000 and 284,807. Across all conditions and sizes, the fastest method varied. On C3, insertion sort was fastest because records were already in exact triage order. The reverse-input C4 made Merge Sort the fastest full-size method. All trials completed within the five minute limit.", "",
            "Required plots: `C2_time_vs_n.svg`, `time_at_10000_by_condition.svg`, `C2_comparisons_vs_n.svg`, `C2_moves_vs_n.svg`, `C2_time_over_nlogn.svg`, and `C2_time_over_n_squared.svg`. The complete method by condition and size table is `C_summary.csv`.", "",
            "## Problem D: top K queue", "",
            "D compares Merge Sort then take K, a size-K min heap, and the bounded insertion buffer on C1 and C2 at n = 100,000 and 284,807. K includes 10, 50, personal K=230, 500, 5,000, n/10, and K=2,068, the smallest measured queue capturing at least half the frauds. Every required row completed.", "",
            "The size-K min heap was fastest at every tested K. No method crossover appeared in this range. The bounded buffer became less suitable as K grew: at full n, its median took 5.28 to 5.37 seconds at K=2,068, 6.15 to 9.47 seconds at K=5,000, and 56.67 to 72.82 seconds at K=28,480. The 2,068 queue captures at least half of all 492 frauds under the required triage score.", "",
            "| Condition | Method | Seconds | Comparisons | Moves | Extra bytes |",
            "|---|---|---:|---:|---:|---:|"]
    for cond in ("C1", "C2"):
        for method in ("Full Sort (Merge Sort)", "Size-K Min-Heap", "Bounded Insertion Buffer"):
            r = next(x for x in dcost if x["condition"] == cond and x["algorithm"] == method)
            txt.append(f"| {cond} | {method} | {float(r['median_seconds']):.6g} | {int(r['comparisons']):,} | {int(r['moves']):,} | {int(r['extra_memory_bytes']):,} |")
    txt += ["", "The complete matrix is `D_summary.csv`. Fraud capture by K is in `fraud_capture_curve.csv` and `fraud_capture_vs_k.svg`; the cost to produce the half-capture queue is in `half_capture_cost.csv`.", "",
            "## Problem E: live feed", "",
            "The data is split into 48 one-hour windows or 288 ten-minute windows by Time. The heap fed only with new records had the lowest cumulative time and lowest extra memory in both cases. Re-ranking everything was slowest; sorting each new window and merging into the top-K queue was between the two.", "",
            "| Window | Strategy | Final cumulative seconds | Peak extra bytes |", "|---|---|---:|---:|"]
    for r in e:
        txt.append(f"| {r['condition']} | {r['algorithm']} | {float(r['median_final_cumulative_seconds']):.6g} | {int(r['peak_extra_memory_bytes']):,} |")
    txt += ["", "Cumulative plots: `E_cumulative_time_1_hour.svg` and `E_cumulative_time_10_minute.svg`. `E_summary.csv` and `live_feed_peak_memory.csv` contain the summary values.", "",
            "## Complexity review and recommendations", "",
            "Insertion Sort takes O(n squared) time in the general case and O(1) auxiliary storage. Merge Sort takes O(n log n) time and O(n) auxiliary storage. Naive Quick Sort has average O(n log n), worst O(n squared), and recursion storage up to O(n). The hardened three-way Quick Sort improves duplicate handling and pivot choice, but still has a quadratic worst case. Heap Sort takes O(n log n) time and O(1) auxiliary storage. The size-K queue heap takes O(n log K) time and O(K) storage; the bounded buffer can take O(nK) time and O(K) storage. For this workload, use Merge Sort for predictable full ranking, a size-K min heap for selective queues, and a heap fed by each new window for live updates.", "",
            "## Scope note", "",
            "The separately assigned F and H sections were reviewed against the updated measurements. This report includes their complexity and recommendation conclusions. Problem G is omitted from this project work as requested. The optional console is present, but its bonus eligibility depends on the instructor's complete A to H assessment.", "",
            "## Output structure", "",
            "`results.csv` contains trial and median rows. `C_summary.csv`, `D_summary.csv`, and `E_summary.csv` provide compact tables. Six C plots, two E plots, and the D fraud-capture plot are saved as SVG files in this folder. `prediction_log.csv` records predictions and outcomes.", ""]
    report.write_text("\n".join(txt), encoding="utf-8")

    # Five-page PDF: profile, C, D/E, theory/recommendations, prediction appendix.
    styles = getSampleStyleSheet()
    title = pstyle(styles["Title"], "title", fontSize=20, leading=23, textColor=colors.HexColor("#173552"), alignment=TA_CENTER, spaceAfter=8)
    h1 = pstyle(styles["Heading1"], "h1", fontSize=14, leading=16, textColor=colors.HexColor("#173552"), spaceBefore=2, spaceAfter=5)
    h2 = pstyle(styles["Heading2"], "h2", fontSize=10, leading=12, textColor=colors.HexColor("#197a63"), spaceBefore=4, spaceAfter=3)
    body = pstyle(styles["BodyText"], "body", fontSize=8, leading=10, spaceAfter=4)
    small = pstyle(body, "small", fontSize=6.3, leading=7.5, spaceAfter=1)
    story = [Paragraph("Fraud Risk Ranking Engine", title), Paragraph("Results report | Roll number 2013 | Section A", body),
             Paragraph("Problem A: know the data", h1),
             Paragraph("The file has 284,807 transactions and 492 frauds. There are no missing, nonnumeric, or non-finite cells. There are 1,081 duplicate full rows; they were kept. Amount has 32,767 distinct values, and the ten most frequent values account for 16.29% of rows. Time is nondecreasing in the raw file. The raw file is neither triage sorted nor its exact reverse. C2 to C6 are constructed conditions; the raw file is C1.", body)]
    band_data = [["Band", "Transactions", "Share", "Frauds", "Fraud share"]] + [[x["band"], f'{int(x["transactions"]):,}', f'{float(x["transaction_share"])*100:.2f}%', x["frauds"], f'{float(x["fraud_share"])*100:.2f}%'] for x in bands]
    story += [table_data(band_data, [0.55*inch, 1.0*inch, .72*inch, .55*inch, .78*inch], 6.4), Spacer(1, 4),
              Image(str(ROOT / "results" / "problem_a" / "risk_band_histogram.png"), width=4.2*inch, height=2.25*inch),
              Paragraph("Problem B: implementation", h2), Paragraph("The C++17 implementation contains five full-sort algorithms, two top-K methods, three live-feed strategies, comparator counters, and correctness checks. Tests cover empty, singleton, duplicates, equivalent keys, and preservation of records.", small),
              Paragraph("Experiment setup: five trials, median summaries, seed 2013, personal K=230. C and D use the five minute limit. All C and D trials completed. `Class` is used only for fraud-capture evaluation.", small), PageBreak()]

    story += [Paragraph("Problem C: full ranking tournament", h1), Paragraph(f"There are 24 prediction rounds. Winner predictions were correct in {correct_w}/24; loser predictions were correct in {correct_l}/24. The full log is included in the appendix and `prediction_log.csv`.", body)]
    c2data = [["n", "Insertion", "Merge", "Naive QS", "Hardened QS", "Heap"]]
    for n in sizes:
        c2data.append([f"{n:,}"] + [f'{float(c2[(n,a)]["median_seconds"]):.4g}' for a in algs])
    story += [table_data(c2data, [0.62*inch] + [0.8*inch]*5, 6.5), Spacer(1, 4)]
    ccharts = [Image(str(fig[key]), width=3.35*inch, height=1.48*inch)
               for key in ("c_time", "c_conditions", "c_comparisons", "c_moves", "c_nlogn", "c_n2")]
    chart_grid = Table([[ccharts[i], ccharts[i+1]] for i in range(0, 6, 2)],
                       colWidths=[3.45*inch, 3.45*inch])
    chart_grid.setStyle(TableStyle([("VALIGN", (0,0), (-1,-1), "MIDDLE"),
                                    ("LEFTPADDING", (0,0), (-1,-1), 0),
                                    ("RIGHTPADDING", (0,0), (-1,-1), 0),
                                    ("TOPPADDING", (0,0), (-1,-1), 0),
                                    ("BOTTOMPADDING", (0,0), (-1,-1), 0)]))
    story.append(chart_grid)
    story += [Paragraph("C2's fastest method was Naive Quick Sort at n=100,000 and 284,807. Across all conditions the winner varied: insertion sort excelled on exact triage order, while Merge Sort led on the reverse input at full size. All requested plots are also available as SVG files beside the CSV results.", small), PageBreak()]

    story += [Paragraph("Problems D and E: selective and live queues", h1),
              Paragraph("D compares full Merge Sort, the size-K min heap, and the bounded insertion buffer across C1/C2 and both sample sizes. K includes 10, 50, 230, 500, 5,000, n/10, plus 2,068 for the half-fraud threshold. The size-K heap was fastest at every tested point. At n=284,807, bounded-buffer time rose from 5.28 to 5.37 seconds at K=2,068, 6.15 to 9.47 seconds at K=5,000, and 56.67 to 72.82 seconds at K=28,480.", body),
              Image(str(fig["d_capture"]), width=4.0*inch, height=1.28*inch)]
    dc = [["Condition", "Method", "Seconds", "Comparisons", "Moves", "Bytes"]]
    for cond in ("C1", "C2"):
        for method in ("Full Sort (Merge Sort)", "Size-K Min-Heap", "Bounded Insertion Buffer"):
            r = next(x for x in dcost if x["condition"] == cond and x["algorithm"] == method)
            dc.append([cond, method.replace("Full Sort (", "").replace(")", ""), f'{float(r["median_seconds"]):.4g}', f'{int(r["comparisons"]):,}', f'{int(r["moves"]):,}', f'{int(r["extra_memory_bytes"]):,}'])
    story += [table_data(dc, [.48*inch, 1.45*inch, .58*inch, .83*inch, .83*inch, .68*inch], 5.7),
              Paragraph("The smallest K capturing at least half the frauds is 2,068 of 492. Full-sort, heap, and buffer median costs for this K are shown above; `half_capture_cost.csv` has exact values.", small),
              Paragraph("For E, the heap fed only by newly arrived transactions was fastest and used the least extra memory for both window sizes. The window inputs are split by Time; memory estimates exclude the preloaded dataset and input windows.", body)]
    ed = [["Window", "Strategy", "Final sec", "Peak bytes"]]
    for r in e:
        ed.append([r["condition"], r["algorithm"].replace("Maintain Size-K ", "").replace("Re-rank All with ", "Re-rank "), f'{float(r["median_final_cumulative_seconds"]):.4g}', f'{int(r["peak_extra_memory_bytes"]):,}'])
    story += [table_data(ed, [.82*inch, 2.5*inch, .75*inch, .75*inch], 6.2),
              Image(str(fig["e_1hour"]), width=3.1*inch, height=1.0*inch), Image(str(fig["e_10min"]), width=3.1*inch, height=1.0*inch), PageBreak()]

    story += [Paragraph("Complexity and recommendations", h1),
              table_data([["Method", "Time", "Extra storage", "Stable"],
                          ["Insertion Sort", "O(n squared)", "O(1)", "Yes"],
                          ["Merge Sort", "O(n log n)", "O(n)", "Yes"],
                          ["Naive Quick Sort", "Average O(n log n), worst O(n squared)", "O(log n) average; O(n) worst stack", "No"],
                          ["Hardened Quick Sort", "Expected O(n log n), worst O(n squared)", "O(log n) average; O(n) worst stack", "No"],
                          ["Heap Sort", "O(n log n)", "O(1)", "No"],
                          ["Size-K min heap", "O(n log K)", "O(K)", "No"],
                          ["Bounded insertion buffer", "O(nK) worst case", "O(K)", "Yes"]], [1.35*inch, 2.35*inch, 1.45*inch, .55*inch], 6.1),
              Paragraph("Use Merge Sort for predictable full ranking, a size-K min heap when only K records are needed, and a heap fed by each new window for the live feed. These choices match the measured performance and memory results.", body),
              Paragraph("Scope and source notes", h2),
              Paragraph("The separately assigned F and H material was reviewed against these refreshed measurements. Problem G is omitted from this project work as requested. The optional local Analyst Console lets an analyst select a queue algorithm and K, view ranked transaction fields, and inspect runtime and comparisons. Its bonus remains conditional on complete A to H assessment.", body),
              Paragraph("AI use disclosure: AI assistance was used to inspect and edit code, organize experiment outputs, and prepare documentation. The student should review the source, measurements, and report before submission and can explain the implementation.", body), PageBreak(),
              Paragraph("Prediction log appendix", h1),
              Paragraph("Each prediction was recorded before its tournament round. The result rows below show the predicted and actual winner and loser. Complete reasoning text is preserved in `prediction_log.csv`.", small)]
    pdata = [["Round", "n", "Condition", "Pred winner", "Actual winner", "Pred loser", "Actual loser", "Reason"]]
    for r in outcomes:
        pdata.append([r["round"], f'{int(r["n"]):,}', r["condition"], r["predicted_winner"], r["actual_winner"], r["predicted_loser"], r["actual_loser"], Paragraph(r["reason"], small)])
    story.append(table_data(pdata, [.38*inch, .48*inch, .42*inch, .78*inch, .78*inch, .78*inch, .78*inch, 3.0*inch], 5.1))
    story += [Spacer(1, 6), Paragraph(f"Prediction score: {correct_w} of 24 winners and {correct_l} of 24 losers predicted correctly. Each round had a written reason in the source log.", body)]

    def footer(canvas, doc):
        canvas.saveState(); canvas.setFont("Helvetica", 7); canvas.setFillColor(colors.HexColor("#667788"))
        canvas.drawString(0.45*inch, 0.3*inch, "Fraud Risk Ranking Engine | 2013-A2-A")
        canvas.drawRightString(7.95*inch, 0.3*inch, f"Page {doc.page} of 5")
        canvas.restoreState()
    doc = SimpleDocTemplate(str(PDF), pagesize=letter, rightMargin=.45*inch, leftMargin=.45*inch,
                            topMargin=.42*inch, bottomMargin=.48*inch, title="Fraud Risk Ranking Engine Report")
    doc.build(story, onFirstPage=footer, onLaterPages=footer)
    print(f"Wrote {report} and {PDF}")


if __name__ == "__main__":
    main()
