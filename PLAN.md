# Assignment requirements and implementation plan

## Specification summary

The assignment requests a reproducible investigation of a triage order over the ULB credit-card transactions. The key is band descending, Amount descending, Time ascending, txn_id ascending; Class is evaluation-only. The score uses full-file standardization for V1–V28, squared z-score sum, then `min(9,max(0,floor(log2(risk))))`. Personal parameters are roll number `s=2013`, random seed 2013, and `K=230`.

Required work:

| Area | Required output |
|---|---|
| A | Band profile/histogram, Amount cardinality/top-ten share, fraud distribution, evidence for existing input conditions |
| B | Insertion, merge, naive Lomuto-last-pivot quicksort, one hardened quicksort, heapsort, size-K min-heap, bounded insertion buffer; correctness tests including edge/tie-heavy inputs |
| C | Five full-sort contenders over C1–C6 and n=1k/10k/100k/full; predictions; timing, counts, memory/stability summary and specified plots |
| D | Full sort, size-K heap, bounded insertion buffer across requested K, n and C1/C2; fraud capture curve and half-capture cost |
| E | 1-hour and 10-minute windows; repeated ranking, incremental size-K heap and merge-new-window strategies; cumulative time and peak extra memory |
| F | Eight required derivations, including inversion count, recurrences, C6 duplicate-group quicksort comparisons and crossover analysis |
| G | Change-request addendum once the course portal posts the change |
| H | Evidence-based recommendations for batch ranking, shift queue and live refresh, including assumptions and failure conditions |
| Submission | C++ source/tests/README, `results.csv`, <=5-page PDF report, prediction log appendix, AI-use disclosure, live demo/viva preparation |

## Repository audit against the assignment

| Requirement | Current state | Limitation / next action |
|---|---|---|
| C++ build and common key | Present; MinGW compile works after adding its `bin` folder to `PATH` | CMake is described but is not installed on this machine; make direct compiler commands primary or install CMake later |
| Problem A data profile | Python pandas/NumPy script, CSV table, JSON summary and histogram generated | Need write the profile and data-quality note into final report; declare `ddof=0` explicitly |
| Problem B full-sort algorithms | All five required algorithms are implemented; tests now exercise each on empty, singleton, duplicate-heavy total-order input, and C6 band-only ordering | Still add larger randomized, sorted and reverse-sorted cases; clarify/count element moves consistently before reporting metrics |
| Problem B Top-K | Heap and bounded insertion implementations exist; differential checks pass for K=0, K<=n and K>n on mixed-priority records | Expand Top-K checks to repeated-key and larger inputs |
| Dataset processing | C++ and Python loaders compute full-file risk bands; local Kaggle CSV is available and ignored by Git | Validate Class values and row IDs robustly; keep `Class` isolated to evaluation |
| C1–C6 input conditions | C++ sample/condition builders now exist; tests check seeded sampling, Time order, pre-ranked/reverse order, record preservation, C5 displacement and C6 comparator | Integrate them into a complete benchmark driver and document C5's random-position procedure |
| Problems C–E experiments | Not implemented | Produce all size/condition/top-K/live-window runs, medians, timeout/DNF handling, memory estimates and `results.csv` |
| Plots and prediction log | Only Problem A histogram exists | Generate requested labelled plots from measurements and fill prediction log before each benchmark round |
| Problem F derivations | Not written | Derive all eight items and compare count predictions with measured runs |
| Problems G and H | Not written | G depends on a later course-portal change; H depends on measured evidence |
| Final submission | No five-page report or complete results file | Generate report source/PDF, include AI-use disclosure and prediction-log appendix, then check page limit |

## Plan and status

1. **Foundation and Problem A:** core sort/selection implementations, dataset loader, Problem A analysis, and profile outputs are present. Expand differential tests and finalize count semantics.
2. **Experiment inputs (in progress):** C++ sampling and C1–C6 condition construction are implemented and covered by deterministic unit tests.
3. **Benchmark driver:** run fresh copies, record comparison/move counts, median of five timings, extra memory/stability and 5-minute DNF. Provide a fast smoke mode and the full assignment matrix.
4. **Problems C–E:** generate `results.csv`, prediction log, charts, top-K fraud-capture results, and live-feed timing/memory results.
5. **Problem F:** derive and document all required recurrences and selection/feed cost models, checking where the measurements agree or differ.
6. **Problems G–H and report:** incorporate the posted change request when supplied; base recommendations on the measured results; write a <=5-page report with AI-use disclosure.
7. **Reproduction pass:** verify build/run commands, keep the dataset out of Git, and prepare a source ZIP and viva notes.

## Measurement integrity and open dependencies

- `creditcard.csv` is available in the local workspace but is intentionally excluded from Git and all submission packages.
- Problem G's change request is posted later by the course and cannot be inferred in advance.
- The assignment's 5-run medians and performance charts will only be reported after running the code on the user's machine and dataset. Never substitute invented measurements.
- The requested PDF report requires a rendering tool; keep source report content editable and record the final page count when generated.
