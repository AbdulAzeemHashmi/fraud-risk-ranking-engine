# Problems C, D, and E Results

## Data and run settings

The experiments use all 284,807 rows in `creditcard.csv` unless a sample size is named. The random seed is 2013, and the personal queue size is K = 230. The `Class` column is used only to evaluate fraud capture after ranking. It is never read by the ranking comparator.

The programs were built as C++17 with MinGW GCC 16.2.0, using `-O2 -Wall -Wextra -Wpedantic`. The host reports Windows NT 10.0.26300.0, x64, and an Intel64 Family 6 Model 142 CPU with 8 logical processors. The hardware query for installed RAM was blocked, so no RAM amount is reported.

Problems C and D use five trials and report the median. A single run stops after 30 seconds and is marked `DNF` if it has not finished. Problem E runs five complete trials without that cutoff. Timing excludes dataset loading and the common input copy. Problem C and D memory figures estimate algorithm auxiliary storage. Problem E memory figures estimate peak extra vector storage and exclude the preloaded dataset and window inputs. They are storage estimates, not operating system memory readings.

The `comparisons` column counts calls to the ranking comparator. The `moves` column counts assignments to algorithm-owned record slots. In C and D summary rows, `trial=0` marks the median row. The detailed trial rows remain in `results.csv`.

## Problem C: Full Ranking Tournament

All five sorting methods were run on C1 through C6 at n = 1,000, 10,000, 100,000, and 284,807. The C summary table has 120 rows, including DNF rows. `results.csv` contains the individual trials and the median rows. The prediction log has a prediction before every round and a result afterward. The predicted winner was correct in 9 of 24 rounds, and the predicted loser was correct in 20 of 24 rounds.

Median time in seconds for C2, the seeded shuffled input:

| n | Insertion Sort | Merge Sort | Naive Quick Sort | Hardened Quick Sort | Heap Sort |
|---:|---:|---:|---:|---:|---:|
| 1,000 | 0.0013955 | 0.0002184 | 0.0001525 | 0.0002263 | 0.0001524 |
| 10,000 | 0.1764851 | 0.0028995 | 0.0020814 | 0.0028865 | 0.0020628 |
| 100,000 | DNF | 0.0609764 | 0.0535929 | 0.0736085 | 0.0867474 |
| 284,807 | DNF | 0.1029780 | 0.0752463 | 0.1057571 | 0.1437046 |

At the larger shuffled sizes, Naive Quick Sort had the lowest measured median among methods that completed. Insertion Sort exceeded the 30 second limit. The winner varies with size and condition, so one input case does not predict every round.

The C plots are:

- `C2_time_vs_n.svg`: log-log execution time versus n.
- `time_at_10000_by_condition.svg`: grouped bars for all six conditions.
- `C2_comparisons_vs_n.svg` and `C2_moves_vs_n.svg`: operation counts versus n.
- `C2_time_over_nlogn.svg` and `C2_time_over_n_squared.svg`: normalized time curves.

The summary table is `C_summary.csv`. It reports median time, comparisons, moves, estimated extra memory, stability, and status for every method, condition, and size. DNF methods have no median time. Their timed out trial is in `results.csv`.

## Problem D: The Shift Queue

The full matrix compares Merge Sort followed by taking K, a size-K min heap, and a bounded insertion buffer. It covers C1 and C2 at n = 100,000 and n = 284,807. K is 10, 50, 230, 500, 5,000, and n/10. `D_summary.csv` contains the median result for every condition, size, K, and method.

The size-K min heap was the fastest method at every tested point where the methods completed. No crossover away from the heap was observed, even at K = n/10. The bounded buffer was slower than the heap at K = 10 and its time grew quickly as K increased. At n = 284,807 and K = 28,480, it reached the 30 second cutoff on both conditions. The results do not establish what happens when K is closer to n.

Under the triage order, the first queue to capture at least half of the 492 frauds has K = 2,068. The measured costs at full data size were:

| Condition | Method | Median seconds | Comparisons | Moves | Estimated extra memory |
|---|---|---:|---:|---:|---:|
| C1 | Full Sort (Merge Sort) | 0.101678 | 4,792,310 | 10,343,704 | 9,113,824 bytes |
| C1 | Size-K Min-Heap | 0.004056 | 527,362 | 367,438 | 66,176 bytes |
| C1 | Bounded Insertion Buffer | 5.103360 | 574,512,669 | 12,345,601 | 66,176 bytes |
| C2 | Full Sort (Merge Sort) | 0.187153 | 4,804,507 | 10,343,704 | 9,113,824 bytes |
| C2 | Size-K Min-Heap | 0.006669 | 518,861 | 354,179 | 66,176 bytes |
| C2 | Bounded Insertion Buffer | 5.083820 | 575,157,234 | 11,700,656 | 66,176 bytes |

The fraud capture data is in `fraud_capture_curve.csv`; the plot is `fraud_capture_vs_k.svg`. The label is used only for this evaluation.

## Problem E: The Live Feed

The full dataset was split into 48 one-hour windows and 288 ten-minute windows by Time. The three strategies were run in five trials. `E_cumulative_time_1_hour.svg` and `E_cumulative_time_10_minute.svg` plot median cumulative time against window number. `E_summary.csv` reports final cumulative time and peak extra memory.

| Window size | Strategy | Final cumulative time, median seconds | Peak extra memory |
|---|---|---:|---:|
| 1 hour | Re-rank All with Merge Sort | 2.221135 | 35,006,336 bytes |
| 1 hour | Maintain Size-K Min-Heap | 0.006033 | 14,720 bytes |
| 1 hour | Merge Sorted Window into Top-K | 0.098345 | 1,055,936 bytes |
| 10 minutes | Re-rank All with Merge Sort | 7.777350 | 20,364,160 bytes |
| 10 minutes | Maintain Size-K Min-Heap | 0.008182 | 14,720 bytes |
| 10 minutes | Merge Sorted Window into Top-K | 0.056847 | 269,504 bytes |

Maintaining the heap was fastest and used the least extra memory for both window sizes. Sorting each new window and merging it into the current queue was faster than re-ranking the full feed. More frequent ten-minute updates increased the total cost of full re-ranking.

## Output files

- `results.csv`: trial-level results and median rows for C, D, and E.
- `C_summary.csv`, `D_summary.csv`, and `E_summary.csv`: compact problem summary tables.
- `prediction_log.csv`: prediction and outcome rows for each C round.
- `half_capture_cost.csv`: cost to produce the smallest queue that captures half the frauds.
- `live_feed_peak_memory.csv`: peak memory estimate for each live-feed strategy.
- SVG files: requested plots, with labels and captions included in each chart.
