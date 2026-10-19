# Project plan and progress

## Goal

Build and study a C++ transaction ranking engine. It ranks transactions by risk and supports full sorting, Top K selection, and live feed updates. Measurements come from the local credit card data file.

## Fixed settings

- Roll number and random seed: `2013`
- Personal queue size: `K = 230`
- Full data size: 284,807 transactions
- Fraud count: 492
- Risk features: V1 through V28
- Sort order: risk band from high to low, Amount from high to low, Time from early to late, then transaction ID from low to high
- Class is used only to evaluate fraud capture after ranking.

## Progress

| Area | Progress | Remaining work |
|---|---|---|
| Problem A: Know Your Data | Done. The Python profile checks data quality, reports risk bands, frauds, and Amount values, and saves a histogram. | Add the profile findings to the final report. |
| Problem B: Build the Contenders | Done. Five full sort methods, a size K heap, a bounded insertion buffer, and C1 to C6 input builders are implemented in C++. Existing tests pass. | Add broader randomized and edge case coverage if time allows. |
| Problem C: Full Ranking Tournament | Done. All five sort contenders ran on C1 to C6 at n = 1,000, 10,000, 100,000, and 284,807. Trial data, medians, predictions, and requested charts are saved. | Use the measurements in the final conclusions. |
| Problem D: Shift Queue | Done. Full sort, size K heap, and bounded insertion buffer ran on C1 and C2 for both required data sizes and K values. Fraud capture and half capture costs are saved. | Use the measured crossover results in the final conclusions. |
| Problem E: Live Feed | Done. All three strategies ran with one hour and ten minute windows. Cumulative time plots and peak memory estimates are saved. | Use the results in the final recommendation. |
| Problem F: Mathematical Case | Not started. | Write all eight derivations and compare theoretical counts with measured counts and times. |
| Problem G: Change Request | Waiting for the course portal to provide the new requirement. | Add the requested update and measurements after the change is available. |
| Problem H: Recommendation | Partly supported by the C, D, and E results. | Recommend methods for daily ranking, shift queues, and live refresh. State assumptions and limits. |
| Final files | Source, tests, Problem A output, C to E measurements, plots, summaries, and report are present. | Complete Problems F and H, add Problem G when available, prepare the final report and demonstration, and check the package. |

## Experiment findings

- Problem C ran 24 condition and size rounds. The prediction log contains a prediction before each round and an outcome after it.
- Problem D found no method crossover from the size K heap within the tested K values. The heap was fastest for all completed points. The bounded buffer reached the 30 second limit for K = n/10 at n = 284,807.
- Under the ranking score, the smallest queue that captured half of the 492 frauds had K = 2,068.
- Problem E found that maintaining the heap had the lowest cumulative time and extra memory for both window sizes.
- See `results/experiments/REPORT.md` for exact measurements and limits.

## Limits and decisions

- `creditcard.csv` stays out of Git and must not be included in a source package.
- The profile uses population standard deviation with `ddof=0`. It checks for missing, nonnumeric, and nonfinite values and does not silently remove or fill rows.
- The profile found 32,767 distinct Amount values. The ten most common values cover 16.291032 percent of rows.
- The raw data is in nondecreasing Time order. It is not already ordered by the full triage key or by its exact reverse.
- The full sort and Top K measurements stop at 30 seconds per run and mark unfinished runs as DNF. These entries are not measured medians.
- Memory values are algorithm storage estimates. The operating system memory amount was not available to the experiment process.
- The programs use MinGW GCC 16.2.0 with C++17 and `-O2 -Wall -Wextra -Wpedantic` for the reported runs.
- No benchmark result, prediction, or recommendation should be invented. Base conclusions on the saved output files.

## Next steps

1. Complete the eight Problem F derivations.
2. Use C, D, and E measurements to write and check the Problem H recommendations.
3. Add Problem G after the course portal provides the change request.
4. Finish a final report within the required length, include the AI use note, and prepare for the demonstration.
5. Check the source package and make sure it does not contain the data file.
