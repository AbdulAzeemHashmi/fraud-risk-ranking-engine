# Project plan and progress

## Goal

Build and study a C++ transaction ranking engine. It ranks transactions by risk and supports full sorting, Top K selection, and live feed updates. Measurements come from the local credit card data file.

## Fixed settings

- Roll number and random seed: `2013`
- Personal queue size: `K = 25 + (2013 mod 226) = 230`
- Full data size: 284,807 transactions
- Fraud count: 492
- Risk features: V1 through V28
- Sort order: risk band from high to low, Amount from high to low, Time from early to late, then transaction ID from low to high
- Class is used only to evaluate fraud capture after ranking.

## Progress

| Area | Status | Notes and remaining work |
|---|---|---|
| Problem A: Know Your Data | Done | The Python profile and the C++ profile agree on all main figures. The findings are in the report. |
| Problem B: Build the Contenders | Done | Five full sorts, a size K heap, a bounded insertion buffer, and the C1 to C6 builders are in C++. Tests pass in the normal build and in the debug build. Broader randomized tests are still optional. |
| Problem C: Full Ranking Tournament | Done | All five sorts ran on C1 to C6 at all four sizes. Trials, medians, predictions, and charts are saved. The report has the plots, the summary table, and the prediction log. |
| Problem D: Shift Queue | Done | Full sort, size K heap, and bounded insertion buffer ran on C1 and C2. Fraud capture and the cost of half capture are saved and are in the report. |
| Problem E: Live Feed | Done | All three strategies ran with one hour and ten minute windows. Time plots and memory estimates are saved and are in the report. |
| Problem F: Mathematical Case | Written in the report | All eight items (M1 to M8) have derivations. Each one is compared with measured counts or times. Read and understand every derivation before the viva. |
| Problem G: Change Request | Waiting | The instructor has not posted the change request. The report has an empty addendum section for it. |
| Problem H: Recommendation | Written in the report | One recommendation each for the nightly batch, the shift queue, and the live refresh, with evidence, an assumption, and a case where it would be wrong. |
| Bonus: Analyst Console | Done | A local web page picks a C++ queue method, sets K, shows the ranked transactions, and shows time and comparison counts. Bonus marks need Problems A to H to be complete and a viva demonstration. |
| Report | First full version done | The PDF is 4 pages (limit is 5). It includes the AI use note and the prediction log as an appendix. Problem G must be added later. |
| README | Updated | New README with project overview, quick start, results, and debug steps. |
| Final package | Not checked yet | See the next steps. |

## Experiment findings

- Problem C ran 24 condition and size rounds. The prediction log has a prediction before each round and the outcome after it. The predicted winner was right in 9 rounds and the predicted loser in 20 rounds.
- On shuffled data at the full size, Naive Quick Sort was fastest (0.075 s). Merge Sort finished every condition at the full size, between 0.054 s and 0.134 s. Insertion Sort did not finish on shuffled data above 10,000 records.
- Problem D found no method that beats the size K heap in the tested K values. The heap was fastest at every completed point. The bounded buffer beats a full sort only for small K (about 50 or less) and hit the 30 second limit at n = 284,807 for K = 5,000 and K = 28,480.
- Under the ranking score, the smallest queue that captures half of the 492 frauds has K = 2,068. The queue of K = 230 captures only 3 frauds.
- Problem E found that the size K heap fed with new records had the lowest cumulative time and the lowest extra memory for both window sizes.
- The M6 formula for Naive Quick Sort on C6 predicted the measured comparisons within 1 percent at n = 1,000, 10,000, and 100,000.
- The inversion count on the C5 sample at the full size is 262,073,628. It was worked out from the saved move counts.
- See `results/experiments/REPORT.md` for exact measurements and limits.

## Limits and decisions

- `creditcard.csv` stays out of Git and must not be included in a source package.
- The profile uses population standard deviation with `ddof=0`. It checks for missing, nonnumeric, and nonfinite values and does not silently remove or fill rows.
- The profile found 32,767 distinct Amount values. The ten most common values cover 16.291032 percent of rows.
- The raw data is in nondecreasing Time order. It is not already ordered by the full ranking order or by its exact reverse.
- The full sort and Top K runs stop at 30 seconds, but the brief says 5 minutes. This is a known difference and the report says so. Unfinished runs are marked DNF and are not medians.
- Memory values are algorithm storage estimates. The operating system memory amount was not available to the experiment process.
- The programs use MinGW GCC 16.2.0 with C++17 and `-O2 -Wall -Wextra -Wpedantic` for the reported runs.
- The tests check edge cases against a reference. The experiment runner checks that every sort output is ordered and that every Top K output matches a Merge Sort reference. There is no separate test that compares every full sort with a reference on every condition and size.
- Hardened Quick Sort was slow on ranked input (1.1 to 1.6 s at the full size). The cause is not proven. A likely reason is the way the three way partition swaps records from the far end.
- The M6 prediction uses group sizes scaled from the full file. The exact group sizes of each sample were not saved.
- The saved peak memory for re-ranking everything differs between the two window sizes (35.0 MB and 20.4 MB). This has not been investigated.
- No benchmark result, prediction, or recommendation should be invented. Base conclusions on the saved output files.

## Next steps

1. Wait for the change request. When it is posted, change the code, run fresh measurements, and write the Problem G addendum in the report.
2. Check every number in the report against `results/experiments` once more.
3. Practice explaining each algorithm, each derivation, and the Analyst Console, because the viva decides how much of the other marks count.
4. Rename the report and the code ZIP to the required name format, `RollNo-A2-Section`.
5. Build the source ZIP. Make sure it has the source, tests, README, and `results.csv`, and does not contain `creditcard.csv` or build files.
6. Optional: add broader randomized tests, and look into the Hardened Quick Sort result and the memory difference noted above.