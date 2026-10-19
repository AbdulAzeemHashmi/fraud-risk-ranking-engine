# Experiment results: Problems C, D, and E

## Run setup

All experiments use roll number and random seed 2013. Personal K is 230. The complete dataset has 284,807 rows and 492 frauds. Runs used C++17, MinGW GCC 16.2.0, `-O2 -Wall -Wextra -Wpedantic`, five trials, and the default 300 second per-trial timeout for C and D. C produced 120 median rows with no DNF rows. D produced 78 median rows with no DNF rows. E used five trials at each window size. Dataset reading and condition construction are outside the timers. `Class` is used only to evaluate queue capture.

The runner checks full ordering and row preservation after every C run. C1 to C5 also match a `std::sort` reference; C6 checks band order and row preservation because equal bands may have any internal order. Comparisons count comparator calls. Moves count record-slot assignments. Memory values are algorithm storage estimates, not process RAM.

## Problem C: full ranking tournament

There are 24 prediction rounds. The predicted winner was correct in 10/24 rounds and the predicted loser in 18/24 rounds. The full per-run prediction and result log is `prediction_log.csv`.

Median seconds on C2, the seeded shuffle:

| n | Insertion | Merge | Naive quicksort | Hardened quicksort | Heap |
|---:|---:|---:|---:|---:|---:|
| 1,000 | 0.0009088 | 0.0001481 | 0.0001452 | 0.0002209 | 8.79e-05 |
| 10,000 | 0.116344 | 0.0029496 | 0.0022026 | 0.0033029 | 0.0020846 |
| 100,000 | 16.0949 | 0.0266999 | 0.0198671 | 0.0288602 | 0.0410585 |
| 284,807 | 111.079 | 0.090557 | 0.0625472 | 0.0890576 | 0.156837 |

Naive Quick Sort had the lowest C2 median at n = 100,000 and 284,807. Across all conditions and sizes, the fastest method varied. On C3, insertion sort was fastest because records were already in exact triage order. The reverse-input C4 made Merge Sort the fastest full-size method. All trials completed within the five minute limit.

Required plots: `C2_time_vs_n.svg`, `time_at_10000_by_condition.svg`, `C2_comparisons_vs_n.svg`, `C2_moves_vs_n.svg`, `C2_time_over_nlogn.svg`, and `C2_time_over_n_squared.svg`. The complete method by condition and size table is `C_summary.csv`.

## Problem D: top K queue

D compares Merge Sort then take K, a size-K min heap, and the bounded insertion buffer on C1 and C2 at n = 100,000 and 284,807. K includes 10, 50, personal K=230, 500, 5,000, n/10, and K=2,068, the smallest measured queue capturing at least half the frauds. Every required row completed.

The size-K min heap was fastest at every tested K. No method crossover appeared in this range. The bounded buffer became less suitable as K grew: at full n, its median took 5.28 to 5.37 seconds at K=2,068, 6.15 to 9.47 seconds at K=5,000, and 56.67 to 72.82 seconds at K=28,480. The 2,068 queue captures at least half of all 492 frauds under the required triage score.

| Condition | Method | Seconds | Comparisons | Moves | Extra bytes |
|---|---|---:|---:|---:|---:|
| C1 | Full Sort (Merge Sort) | 0.187189 | 4,792,310 | 10,343,704 | 9,113,824 |
| C1 | Size-K Min-Heap | 0.0087385 | 527,362 | 367,438 | 66,176 |
| C1 | Bounded Insertion Buffer | 5.27561 | 574,512,669 | 12,345,601 | 66,176 |
| C2 | Full Sort (Merge Sort) | 0.195027 | 4,804,507 | 10,343,704 | 9,113,824 |
| C2 | Size-K Min-Heap | 0.0070421 | 518,861 | 354,179 | 66,176 |
| C2 | Bounded Insertion Buffer | 5.37372 | 575,157,234 | 11,700,656 | 66,176 |

The complete matrix is `D_summary.csv`. Fraud capture by K is in `fraud_capture_curve.csv` and `fraud_capture_vs_k.svg`; the cost to produce the half-capture queue is in `half_capture_cost.csv`.

## Problem E: live feed

The data is split into 48 one-hour windows or 288 ten-minute windows by Time. The heap fed only with new records had the lowest cumulative time and lowest extra memory in both cases. Re-ranking everything was slowest; sorting each new window and merging into the top-K queue was between the two.

| Window | Strategy | Final cumulative seconds | Peak extra bytes |
|---|---|---:|---:|
| 1-hour | Maintain Size-K Min-Heap | 0.0045345 | 14,720 |
| 1-hour | Merge Sorted Window into Top-K | 0.126549 | 1,055,936 |
| 1-hour | Re-rank All with Merge Sort | 2.59802 | 35,006,336 |
| 10-minute | Maintain Size-K Min-Heap | 0.0080495 | 14,720 |
| 10-minute | Merge Sorted Window into Top-K | 0.0658661 | 269,504 |
| 10-minute | Re-rank All with Merge Sort | 8.66813 | 20,364,160 |

Cumulative plots: `E_cumulative_time_1_hour.svg` and `E_cumulative_time_10_minute.svg`. `E_summary.csv` and `live_feed_peak_memory.csv` contain the summary values.

## Complexity review and recommendations

Insertion Sort takes O(n squared) time in the general case and O(1) auxiliary storage. Merge Sort takes O(n log n) time and O(n) auxiliary storage. Naive Quick Sort has average O(n log n), worst O(n squared), and recursion storage up to O(n). The hardened three-way Quick Sort improves duplicate handling and pivot choice, but still has a quadratic worst case. Heap Sort takes O(n log n) time and O(1) auxiliary storage. The size-K queue heap takes O(n log K) time and O(K) storage; the bounded buffer can take O(nK) time and O(K) storage. For this workload, use Merge Sort for predictable full ranking, a size-K min heap for selective queues, and a heap fed by each new window for live updates.

## Scope note

The separately assigned F and H sections were reviewed against the updated measurements. This report includes their complexity and recommendation conclusions. Problem G is omitted from this project work as requested. The optional console is present, but its bonus eligibility depends on the instructor's complete A to H assessment.

## Output structure

`results.csv` contains trial and median rows. `C_summary.csv`, `D_summary.csv`, and `E_summary.csv` provide compact tables. Six C plots, two E plots, and the D fraud-capture plot are saved as SVG files in this folder. `prediction_log.csv` records predictions and outcomes.
