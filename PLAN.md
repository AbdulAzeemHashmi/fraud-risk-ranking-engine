# Completion notes

## Project settings

- Roll number and experiment seed: `2013`
- Personal queue size: `K = 25 + (2013 mod 226) = 230`
- Dataset: 284,807 transactions, including 492 frauds
- Ranking: risk band high to low, Amount high to low, Time early to late, then transaction ID low to high
- `Class` is used only to measure fraud capture after ranking.

## Work completed

| Area | Work and result |
|---|---|
| A | Dataset profile, risk band table and histogram, amount frequency summary, fraud spread, cleaning checks, and raw input properties are saved under `results/problem_a`. |
| B | C++ sorting and top K methods are implemented. Tests cover edge cases, including equivalent keys. C experiments validate ordering and record preservation; C1 to C5 also match a trusted sort reference. |
| C | All five sorting methods ran across six conditions and four sizes using five trials and the default 300 second timeout. All 120 median rows are complete with no DNF. The six requested plots, summaries, and all 24 prediction outcomes are saved in `results/experiments_final_v2`. |
| D | Full sort, size K min heap, and bounded insertion buffer ran on C1 and C2 at both requested sizes and all required K values. All 78 summary rows completed. The smallest queue capturing at least half the frauds has K=2,068. |
| E | All three live feed strategies ran for five trials with 1 hour and 10 minute windows. Cumulative plots and peak storage estimates are saved. |
| F and H | The existing report's complexity and recommendation material was reviewed against the code and refreshed measurements. The new report states the measured tradeoffs. No separate handoff file from Claude was present in the workspace. |
| G | Excluded from this work as requested. |
| Report and package | Generated the five page `2013-A2-A.pdf`, root `results.csv`, and the section A source archive `2013-A2-A.zip`. The archive excludes the dataset and build products. |

## Measurement details

All C and D trials completed within five minutes. The seeded C2 shuffle was fastest with Naive Quick Sort at n=100,000 and n=284,807. The fastest full sort changed with input condition. For D, the size K min heap was fastest at every tested point. For E, the heap fed by each new window was fastest and used the least estimated extra memory.

The full trial data and derived results are under `results/experiments_final_v2`. The root `results.csv` is the submission copy. `analysis/summarize_results.py` creates compact C, D, and E tables. `analysis/build_report.py` creates the Markdown report, report figures, and five page PDF from those results.

## F and H handoff note

The prior report PDF contained F and H material, but no separate Claude handoff files were found in the project. The refreshed report includes complexity bounds and conclusions supported by the current measurements. The student should compare this concise summary with Claude's final F and H sections before submitting if those sections are delivered separately.

## Local run results

- C summary: 120 rows, all completed.
- D summary: 78 rows, all completed.
- E summary: 6 strategy and window summaries.
- Prediction log: 24 rounds, 10 winner predictions and 18 loser predictions were correct.
- PDF report: 5 pages.
- Package: `2013-A2-A.zip`.
