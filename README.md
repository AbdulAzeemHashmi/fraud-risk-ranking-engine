# The Midnight Queue

The Midnight Queue ranks payment transactions by estimated risk. Analysts can review only a limited number of transactions during each shift, so the highest ranked items should appear first.

This project uses the Kaggle credit card fraud data. It calculates a risk band for each transaction and orders transactions by:

1. Risk band from highest to lowest.
2. Amount from highest to lowest.
3. Time from earliest to latest.
4. Transaction ID from lowest to highest.

The transaction class identifies fraud for evaluation only. It is not used to calculate risk or to order transactions.

## Risk score and data checks

The input CSV has columns `Time`, `V1` through `V28`, `Amount`, and `Class`. For each of V1 through V28, the program calculates the mean and population standard deviation from the full file. It converts each feature value to a z score, squares the 28 z scores, and adds them to get the risk score. The risk band is the base 2 logarithm of that score, rounded down and limited to the range 0 to 9. A zero risk score is assigned to band 0.

The Python profile found no missing, nonnumeric, or nonfinite values. It found 1,081 repeated full data rows. These rows are kept because each row is a separate transaction and receives its own transaction ID.

## Personal settings

- Roll number and random seed: `2013`
- Shift queue size: `K = 25 + (2013 mod 226) = 230`
- Dataset file: `creditcard.csv`

Keep the dataset in the project root when running the commands below. The file is excluded from Git and should not be included in a source package.

## Current features

### Transaction processing

The C++ loader reads the Kaggle CSV file, checks its columns and numeric fields, and calculates the mean and population standard deviation for features V1 through V28. It standardizes each feature, sums the squared standardized values, and maps the result to a risk band from 0 to 9.

### Ranking and selection algorithms

The C++ library includes these implementations:

- Insertion Sort
- Merge Sort
- Naive Quick Sort using the last item as the Lomuto pivot
- Hardened Quick Sort using a median of three pivot and three way partition
- Heap Sort
- Top K using a size K min heap
- Top K using a bounded insertion buffer

The C++ condition builder creates time ordered samples and the C1 through C6 input conditions with random seed 2013. Unit tests check the sorting methods, Top K selection, sample repeatability, and condition construction.

### Input condition methods

For each required sample size, the builder selects rows uniformly without replacement and keeps them in Time order. It then creates these inputs:

- C1: sampled rows in Time order.
- C2: C1 shuffled with the seeded random number generator.
- C3: C1 ordered by the full triage key using the project's Merge Sort.
- C4: the exact reverse of C3.
- C5: C3 with one percent of records selected and placed at different random positions. Other records keep their relative order.
- C6: the C1 input with a band-only comparison key.

The benchmark driver for these conditions is still to be written.

### Problem A profile

The Python analysis uses pandas and NumPy to profile the full dataset. It checks for missing, nonnumeric, and nonfinite values. It does not silently drop or fill rows. Matplotlib creates the risk band histogram.

The current profile contains 284,807 transactions and 492 frauds. It finds 32,767 distinct Amount values. The ten most frequent Amount values account for 16.291032 percent of transactions. The raw file is in nondecreasing Time order.

The standard deviation uses the population formula with `ddof=0`. This choice is recorded in the profile output.

## Build and run on Windows

Open PowerShell in the project folder:

```powershell
cd "C:\Users\Azeem\Documents\fraud-risk-ranking-engine"
$env:Path = "C:\msys64\mingw64\bin;$env:Path"
New-Item -ItemType Directory -Force build | Out-Null
```

Add the MinGW folder to `PATH` so the compiler and its runtime libraries can be found. Then compile the profile program and the tests:

```powershell
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -I include src\profile.cpp -o build\queue_profile.exe
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -I include tests\test_algorithms.cpp -o build\queue_tests.exe
```

Run the tests, the C++ data profile, and the Python profile:

```powershell
.\build\queue_tests.exe
.\build\queue_profile.exe .\creditcard.csv
python analysis\problem_a.py creditcard.csv --out results\problem_a
```

The C++ profile program prints the transaction count, fraud count, Amount statistics, risk band counts, and checks the triage ordering. The Python command recreates the Problem A table, summary, and histogram.

The project also has a `CMakeLists.txt` for CMake 3.16 or newer. Direct MinGW commands above work without CMake.

## Project files

```text
fraud-risk-ranking-engine/
|-- analysis/
|   `-- problem_a.py
|-- include/
|   `-- midnight_queue/
|       |-- algorithms.hpp
|       |-- dataset.hpp
|       |-- experiment_data.hpp
|       `-- record.hpp
|-- results/
|   `-- problem_a/
|       |-- risk_band_distribution.csv
|       |-- risk_band_histogram.png
|       `-- summary.json
|-- src/
|   `-- profile.cpp
|-- tests/
|   `-- test_algorithms.cpp
|-- .gitignore
|-- CMakeLists.txt
|-- PLAN.md
`-- README.md
```

### File guide

| Path | Purpose |
|---|---|
| `include/midnight_queue/record.hpp` | Transaction record and triage comparison rules |
| `include/midnight_queue/algorithms.hpp` | Sorting algorithms, Top K methods, and comparison and move counters |
| `include/midnight_queue/dataset.hpp` | CSV loading, feature standardization, risk score, and risk band calculation |
| `include/midnight_queue/experiment_data.hpp` | Seeded sampling and C1 through C6 input condition construction |
| `src/profile.cpp` | C++ profile command line program |
| `tests/test_algorithms.cpp` | C++ correctness tests |
| `analysis/problem_a.py` | Python data checks, Problem A statistics, and histogram creation |
| `results/problem_a/risk_band_distribution.csv` | Risk band and fraud count table |
| `results/problem_a/summary.json` | Dataset checks, Amount statistics, and input order evidence |
| `results/problem_a/risk_band_histogram.png` | Labelled risk band chart |
| `CMakeLists.txt` | Optional CMake build setup |
| `PLAN.md` | Work completed, known limits, and remaining work |
| `.gitignore` | Keeps the dataset and local build files out of Git |

## Work still needed

The full benchmark program is not ready. The requested experiments for Problems C, D, and E, the full measurement file, prediction log, additional plots, mathematical explanations, recommendations, and final report still need to be completed. See [PLAN.md](PLAN.md) for the full checklist. Do not treat missing measurements as completed results.
