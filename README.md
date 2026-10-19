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

The C++ condition builder creates time ordered samples and the C1 through C6 input conditions with random seed 2013. The C++ experiment runner measures the sort and Top K methods and the live feed strategies. Unit tests check the sorting methods, Top K selection, sample repeatability, and condition construction.

### Input condition methods

For each required sample size, the builder selects rows uniformly without replacement and keeps them in Time order. It then creates these inputs:

- C1: sampled rows in Time order.
- C2: C1 shuffled with the seeded random number generator.
- C3: C1 ordered by the full triage key using the project's Merge Sort.
- C4: the exact reverse of C3.
- C5: C3 with one percent of records selected and placed at different random positions. Other records keep their relative order.
- C6: the C1 input with a band-only comparison key.

The benchmark driver runs all five sorting methods on each condition and size. It records up to five trial times, medians, comparisons, moves, estimated extra memory, stability, and prediction results. A run that takes longer than 30 seconds is marked DNF.

### Problems C, D, and E results

The completed run is in `results/experiments`. Problem C includes C1 through C6 and n = 1,000, 10,000, 100,000, and 284,807. Problem D compares full sorting, a size K min heap, and a bounded insertion buffer for the requested K values. Problem E compares three live feed strategies with one hour and ten minute windows.

The files `results.csv`, `C_summary.csv`, `D_summary.csv`, and `E_summary.csv` contain measurements. The prediction log records the prediction before each C round and its result afterward. SVG charts contain the requested timing, operation count, and fraud capture plots. `results/experiments/REPORT.md` explains the findings and limits.

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
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -I include src\experiment_runner.cpp -o build\queue_experiments.exe
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -I include src\analyst_console.cpp -o build\analyst_console.exe
```

Run the tests, the C++ data profile, and the Python profile:

```powershell
.\build\queue_tests.exe
.\build\queue_profile.exe .\creditcard.csv
python analysis\problem_a.py creditcard.csv --out results\problem_a
```

The C++ profile program prints the transaction count, fraud count, Amount statistics, risk band counts, and checks the triage ordering. The Python command recreates the Problem A table, summary, and histogram.

The project also has a `CMakeLists.txt` for CMake 3.16 or newer. Direct MinGW commands above work without CMake.

Run the C, D, and E experiments from PowerShell:

```powershell
.\build\queue_experiments.exe .\creditcard.csv --problem all --out results\experiments_new
```

Use a new output folder for a fresh run. If you run the problems separately, use the same output folder so each problem appends its rows:

```powershell
.\build\queue_experiments.exe .\creditcard.csv --problem C --out results\experiments_new
.\build\queue_experiments.exe .\creditcard.csv --problem D --out results\experiments_new
.\build\queue_experiments.exe .\creditcard.csv --problem E --out results\experiments_new
```

The `--smoke` option runs a small data sample to check that the commands and output files work. Smoke results are not assignment measurements.

## Analyst Console

The local web console lets an analyst choose Full Sort with Merge Sort, Size-K Min-Heap, or Bounded Insertion Buffer. Set K, build the queue, and inspect transaction ID, risk band, Amount, and Time. The console shows the run time, comparison count, and move count. The Python file only serves the page and calls the C++ executable. Ranking remains in C++.

After compiling `analyst_console.exe`, start the page from the project folder:

```powershell
python analysis\analyst_console.py
```

Open `http://127.0.0.1:8000` in a browser. Keep the PowerShell window open while using the page. Press Ctrl+C in that window to stop it. The server only listens on the local computer. The default queue size is K = 230. Large K values can make the bounded insertion buffer take longer.

To use another data file, port, or C++ executable, pass command line options:

```powershell
python analysis\analyst_console.py --data .\creditcard.csv --exe .\build\analyst_console.exe --port 8000
```

## Debug C++ code

Use a debug build when you want readable error messages and to step through the C++ code. In the VS Code terminal, open PowerShell in the project folder and add MinGW to `PATH`:

```powershell
cd "C:\Users\Azeem\Documents\fraud-risk-ranking-engine"
$env:Path = "C:\msys64\mingw64\bin;$env:Path"
New-Item -ItemType Directory -Force build | Out-Null
```

Compile the program or tests with debug symbols. The `-g` option adds symbols, and `-O0` keeps the code easier to follow while debugging:

```powershell
g++ -std=c++17 -O0 -g -Wall -Wextra -Wpedantic -I include src\profile.cpp -o build\queue_profile_debug.exe
g++ -std=c++17 -O0 -g -Wall -Wextra -Wpedantic -I include tests\test_algorithms.cpp -o build\queue_tests_debug.exe
g++ -std=c++17 -O0 -g -Wall -Wextra -Wpedantic -I include src\experiment_runner.cpp -o build\queue_experiments_debug.exe
g++ -std=c++17 -O0 -g -Wall -Wextra -Wpedantic -I include src\analyst_console.cpp -o build\analyst_console_debug.exe
```

Run the program or test executable directly to see its output in the terminal:

```powershell
.\build\queue_tests_debug.exe
.\build\queue_profile_debug.exe .\creditcard.csv
.\build\queue_experiments_debug.exe .\creditcard.csv --problem C --smoke --out results\debug_run
```

To step through code in VS Code:

1. Install the Microsoft C/C++ extension and make sure the MinGW debugger `C:\msys64\mingw64\bin\gdb.exe` exists.
2. Open the C++ file you want to inspect and click next to a line number to add a breakpoint.
3. Open Run and Debug, choose the C++ debug configuration, and select the debug executable as the program.
4. Set program arguments when needed. For the profile program, use `creditcard.csv`.
5. Start debugging. Use Continue, Step Over, Step Into, and Step Out to follow the program. Check variables in the Variables panel and output in the Debug Console or terminal.

If VS Code does not have a MinGW launch configuration, create `.vscode/launch.json` with this configuration. Run it from the project root:

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Debug transaction profile",
      "type": "cppdbg",
      "request": "launch",
      "program": "${workspaceFolder}/build/queue_profile_debug.exe",
      "args": ["${workspaceFolder}/creditcard.csv"],
      "stopAtEntry": false,
      "cwd": "${workspaceFolder}",
      "environment": [],
      "externalConsole": false,
      "MIMode": "gdb",
      "miDebuggerPath": "C:/msys64/mingw64/bin/gdb.exe",
      "setupCommands": [
        {
          "description": "Enable GDB pretty printing",
          "text": "-enable-pretty-printing",
          "ignoreFailures": true
        }
      ]
    }
  ]
}
```

To debug the tests instead, change `program` to `${workspaceFolder}/build/queue_tests_debug.exe` and remove the `args` field. Header files are compiled through the `.cpp` program or test that includes them. Add breakpoints in a header and start the matching `.cpp` debug configuration.

To debug a console queue run without the web page, run the debug executable with the data path, method, and K:

```powershell
.\build\analyst_console_debug.exe .\creditcard.csv size_k_min_heap 230
```

To debug a request from the web page, start the server with the debug executable:

```powershell
python analysis\analyst_console.py --exe .\build\analyst_console_debug.exe
```

### Debug the Python profile

Install the Python extension in VS Code and select the Python environment that has pandas, NumPy, and Matplotlib. Open `analysis/problem_a.py`, click next to a line number to add a breakpoint, and choose Debug Python File. Set these script arguments in the launch settings:

```text
creditcard.csv --out results/problem_a
```

You can also use Python's command line debugger from PowerShell:

```powershell
python -m pdb analysis\problem_a.py creditcard.csv --out results\problem_a
```

At the `pdb` prompt, enter `n` to run the next line, `s` to enter a function, `p variable_name` to print a value, and `c` to continue to the next breakpoint or program end.

## Project files

```text
fraud-risk-ranking-engine/
|-- analysis/
|   |-- analyst_console.py
|   `-- problem_a.py
|-- include/
|   `-- midnight_queue/
|       |-- algorithms.hpp
|       |-- dataset.hpp
|       |-- experiment_data.hpp
|       |-- svg_plot.hpp
|       `-- record.hpp
|-- results/
|   |-- experiments/
|   |   |-- C_summary.csv
|   |   |-- D_summary.csv
|   |   |-- E_summary.csv
|   |   |-- REPORT.md
|   |   |-- results.csv
|   |   `-- prediction_log.csv
|   `-- problem_a/
|       |-- risk_band_distribution.csv
|       |-- risk_band_histogram.png
|       `-- summary.json
|-- src/
|   |-- analyst_console.cpp
|   |-- experiment_runner.cpp
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
| `include/midnight_queue/svg_plot.hpp` | SVG line and grouped bar charts for the experiments |
| `src/profile.cpp` | C++ profile command line program |
| `src/experiment_runner.cpp` | C++ benchmark runner for Problems C, D, and E |
| `src/analyst_console.cpp` | C++ queue builder used by the local analyst page |
| `tests/test_algorithms.cpp` | C++ correctness tests |
| `analysis/analyst_console.py` | Local web page server and launcher for the C++ queue builder |
| `analysis/problem_a.py` | Python data checks, Problem A statistics, and histogram creation |
| `results/problem_a/risk_band_distribution.csv` | Risk band and fraud count table |
| `results/problem_a/summary.json` | Dataset checks, Amount statistics, and input order evidence |
| `results/problem_a/risk_band_histogram.png` | Labelled risk band chart |
| `results/experiments/results.csv` | Trial measurements and median rows for Problems C, D, and E |
| `results/experiments/C_summary.csv` | Full sort summary by condition and size |
| `results/experiments/D_summary.csv` | Top K summary by condition, size, K, and method |
| `results/experiments/E_summary.csv` | Final live feed time and peak memory summary |
| `results/experiments/REPORT.md` | Plain language summary of the experiment results |
| `CMakeLists.txt` | Optional CMake build setup |
| `PLAN.md` | Work completed, known limits, and remaining work |
| `.gitignore` | Keeps the dataset and local build files out of Git |

## Work still needed

Problems C, D, and E are complete and their measured outputs are in `results/experiments`. Problems F, G, and H remain. Problem G depends on a change request from the course portal. See [PLAN.md](PLAN.md) for the remaining work.
