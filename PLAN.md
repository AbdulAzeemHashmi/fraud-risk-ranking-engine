# Project plan and progress

## Goal

Build and study a C++ transaction ranking engine. The system should rank transactions by risk and support full sorting, Top K selection, and later updates from a live feed. Results must come from running the code on the local credit card data.

## Fixed settings

- Roll number and random seed: `2013`
- Queue size: `K = 230`
- Full dataset size: 284,807 transactions
- Fraud count in the current data file: 492
- Risk features: V1 through V28
- Sort order: risk band descending, Amount descending, Time ascending, transaction ID ascending
- Class is used only to count frauds after ranking.

## Current progress

| Area | Progress | Remaining work |
|---|---|---|
| Problem A: Know Your Data | Done. The Python profile checks data quality, reports risk bands and fraud counts, reports Amount statistics, and saves a labelled histogram. | Add the profile findings to the final report. |
| Problem B: Build the Contenders | The five full sort methods, size K heap, and bounded insertion buffer are implemented in C++. The tests cover empty input, one item, repeated keys, Top K sizes, seeded sampling, and C1 to C6 construction. | Add larger randomized, sorted, and reverse sorted test cases. Review comparison and move counts before using them in result tables. |
| Experiment inputs | The C++ code samples rows in Time order and builds C1 to C6 with the personal seed. | Add these inputs to the benchmark program. |
| Problem C: Full Ranking Tournament | Not started. | Run all five full sort methods for all required input conditions and sizes. Save five run medians, counts, memory estimates, stability, and predictions. |
| Problem D: Shift Queue | Not started. | Compare full sorting, the size K heap, and the bounded buffer across the required queue sizes, data sizes, and input conditions. Measure fraud capture. |
| Problem E: Live Feed | Not started. | Compare full re-ranking, a maintained heap, and sorting plus merging for one hour and ten minute windows. Save cumulative time and extra memory. |
| Problem F: Mathematical Case | Not started. | Write all eight derivations and compare theoretical predictions with measured counts and times. |
| Problem G: Change Request | Waiting for the course portal to provide the change. | Add the requested report update and new measurements after the change is posted. |
| Problem H: Recommendation | Not started. | Recommend methods for daily ranking, the shift queue, and live refresh using measured results. State assumptions and cases where each choice could be wrong. |
| Final files | README, source code, tests, and Problem A outputs are present. | Create `results.csv`, the prediction log, requested charts, and a report of no more than five pages. Include a short AI use note and prepare for the demonstration. |

## Limits and decisions

- The local `creditcard.csv` file is not tracked in Git and must not be included in the source package.
- The profile uses population standard deviation with `ddof=0`. It checks for missing, nonnumeric, and nonfinite data and does not silently remove or fill rows.
- The current profile found 32,767 distinct Amount values. The ten most common Amount values cover 16.291032 percent of all transactions.
- The raw data is in nondecreasing Time order. It is not already sorted by the complete triage key and is not the exact reverse of that order.
- The C++ programs use MinGW. Add `C:\msys64\mingw64\bin` to PowerShell `PATH` so MinGW can find its runtime libraries.
- Problem G cannot be completed until the course provides its new requirement.
- No benchmark result, prediction, or recommendation should be invented. Record the computer and software details when the full runs are performed.

## Next steps

1. Expand the correctness tests and confirm the comparison and move counting rules.
2. Build a benchmark program that uses fresh input copies, runs five trials, records median time, and stops a run after five minutes.
3. Run Problems C, D, and E. Save all measurements in `results.csv` and record predictions before each run.
4. Create the requested plots from measured data. Every plot needs clear axis labels and units, a legend, and a caption.
5. Write the Problem F derivations and use the results to support Problem H recommendations.
6. Add the Problem G update after the new requirement is posted.
7. Write and check the final report, package source and tests without the data file, and prepare for the live demonstration.
