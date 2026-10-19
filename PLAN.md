# Assignment requirements and implementation plan

## Specification summary

The assignment requests a reproducible investigation of a triage order over the ULB credit-card transactions. The key is band descending, Amount descending, Time ascending, txn_id ascending; Class is evaluation-only. The score uses full-file standardization for V1–V28, squared z-score sum, then `min(9,max(0,floor(log2(risk))))`. Personal parameters are roll number `s=2013`, random seed 2013, and `K=46`.

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

## Plan and status

1. **Project foundation (in progress):** C++17 build, common transaction/key/metrics model, algorithms, and correctness tests.
2. **Input pipeline:** CSV parser, full-file standardization/risk bands, stable row IDs, profiling and input-condition builders. Require the locally downloaded Kaggle file; do not bundle it.
3. **Benchmark design:** reproducible seeded samples and transformations; count comparisons and moves with one documented definition; five-run medians; guard each run at five minutes and report DNF.
4. **Problems C–E:** generate every requested measurement and machine-readable results; generate clearly labelled plots with axes/units, legends and captions from measured results.
5. **Problem F:** write derivations and connect predictions/crossovers to actual counts and timings.
6. **Problems G–H and report:** integrate posted change request when available, write evidence-based recommendations and AI-use disclosure, fit report to five pages, and prepare viva notes.
7. **Reproduction pass:** document exact build/run commands, verify outputs and package source/tests without the dataset.

## Measurement integrity and open dependencies

- No dataset is present in the repository; dataset-specific profiling and results must wait for `creditcard.csv` at run time.
- Problem G's change request is posted later by the course and cannot be inferred in advance.
- The assignment's 5-run medians and performance charts will only be reported after running the code on the user's machine and dataset. Never substitute invented measurements.
- The requested PDF report requires a rendering tool; keep source report content editable and record the final page count when generated.
