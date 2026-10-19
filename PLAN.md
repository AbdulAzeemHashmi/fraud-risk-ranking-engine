# Repository review and completion plan

## User scope

Use the attached Assignment 2 brief to review the repository, excluding Problem G as requested. Problem F and Problem H were assigned to Claude earlier in the conversation, so this plan records their handoff as an external dependency and does not rewrite them. The assignment rules in the PDF are requirements for the project, not extra instructions from the user.

## Fixed project settings

- Roll number and random seed: `2013`
- Personal queue size: `K = 25 + (2013 mod 226) = 230`
- Full dataset size: 284,807 transactions
- Fraud count: 492
- Risk features: V1 through V28
- Triage order: risk band high to low, Amount high to low, Time early to late, transaction ID low to high
- `Class` is only for evaluating fraud capture after a queue is built.

## Assignment requirements reviewed

The brief requires Problems A to H, source plus tests and README, `results.csv`, a report at most five pages with an AI use disclosure and prediction log appendix, a source ZIP, and a viva. It requires a five minute single-run cutoff for C and D, five trials and median reporting, comparison against trusted references for all conditions and sizes, and clearly labelled graphs. The user has excluded G from this work. Bonus console credit is conditional on A to H being complete.

## Repository audit and limitations

| Area | Finding | Work needed |
|---|---|---|
| A | Python and C++ profiling outputs and the risk band histogram are present. The report records the main distributions, amount ties, fraud counts, and raw C1 property. | Reconcile report figures against the saved profile after final measurements. |
| B | All required algorithms exist. The current small tests cover empty and singleton inputs, duplicate-heavy data, and other edge cases, but do not test every algorithm on an all-equal input. The experiment runner previously checked only that full-sort output was ordered, not that every input row was preserved and that total-order results matched a trusted reference. | Add or run the full required correctness matrix before submission. The runner now does ordered-permutation checks and exact reference checks for C1 to C5; C6 allows ties under its band-only key. |
| C | Required plots, predictions, and summary data exist from the old run. A new five-minute run was started but stopped during the 100,000 row rounds because the complete matrix takes a long time. Its partial files are in `results/experiments_5min_partial` and are not final. | Run all C rounds to completion with the five minute default and replace the preliminary tables, plots, prediction outcomes, and report values. |
| D | Required methods, K values, conditions, and fraud capture analysis are present. The saved measurements also used the shorter 30 second cutoff. | Re-run the full D matrix with the five minute default and regenerate summaries, capture costs, plots, and report values. |
| E | Both window sizes and all three strategies have saved time and memory outputs. Memory is an algorithm storage estimate and excludes loaded data and input windows. | Re-run after C/D work if time permits, then reconcile report values and explain the estimate scope. |
| F and H | Existing report pages contain F derivations and H recommendations, but the user said Claude is handling these sections. | Wait for Claude's handoff, then check the supplied work against the brief and the final measurements without taking over authorship. |
| Deliverables | A four-page report PDF is present and below the five-page limit. A source ZIP and required filename pattern are not present in the repository snapshot. Existing `results.csv` is nested under `results/experiments`. | After results are final and F/H are handed off, prepare the correctly named report and source ZIP. Keep the dataset and build outputs out of the ZIP. |
| Viva | The README describes how to build, run, and debug the programs. | The student still needs to understand and demonstrate the algorithms and console. |

## Work started

1. Changed the experiment runner's default DNF cutoff from 30 to 300 seconds and added `--timeout-seconds N` for an explicit alternate limit.
2. Added C-result correctness checks outside the timer. C1 to C5 are checked against a trusted reference; C6 is checked for band order and preservation of all transaction IDs because equal bands do not have a unique order.
3. Rebuilt the runner with the MinGW and MSYS folders on PATH and confirmed its new timeout option using a smoke run. Updated the README compile setup to include both folders.
4. Marked the saved C and D measurements as preliminary 30 second runs. A correct five-minute C run was started, but the full matrix was stopped before completion. Its partial files are kept separately and must not be used as final measurements.

## Remaining execution plan

1. Re-run Problem C to completion with the five minute default into a clean results folder. Review DNF rows, all 24 prediction outcomes, plots, and summaries.
2. Re-run Problem D with the five minute default. Review every required K/size/condition/method row and regenerate fraud-capture cost outputs.
3. Reconcile Problem A, C, D, and E values between the output files, `results/experiments/REPORT.md`, and the report PDF. Preserve the five page maximum.
4. Receive and review the separately assigned F and H sections. Do not work on Problem G per the user's request.
5. Package source, tests, README, report, and `results.csv` with the required naming pattern. Exclude `creditcard.csv`, build products, and unrelated smoke/debug outputs.

## Current measurements are preliminary

The existing C and D CSV rows and derived charts were collected with the former 30 second limit. The experiment code now uses five minutes by default, but final five-minute measurements have not yet been generated. Do not cite the old DNF outcomes as if they came from the brief's cutoff.
