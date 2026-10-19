# The Midnight Queue — C++ Assignment 2

C++ implementation and experiments for the Design and Analysis of Algorithms assignment. The PDF requirements are treated as the specification; all executable code is C++ as requested.

## Personal parameters

- Roll number / random seed: `2013`
- Shift capacity: `K = 25 + (2013 mod 226) = 230`
- Dataset: obtain `creditcard.csv` from the Kaggle page in the assignment. Do not commit or include the dataset.

## Build

Requires a C++17 compiler and CMake 3.16 or newer.

```powershell
$env:Path = "C:\msys64\mingw64\bin;$env:Path"
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

Or build the current targets directly with MinGW from PowerShell:

```powershell
$env:Path = "C:\msys64\mingw64\bin;$env:Path"
New-Item -ItemType Directory -Force build | Out-Null
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -I include src\profile.cpp -o build\queue_profile.exe
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -I include tests\test_algorithms.cpp -o build\queue_tests.exe
.\build\queue_tests.exe
.\build\queue_profile.exe .\creditcard.csv
```

The Problem A profiler accepts the dataset path as a command-line argument. Benchmark commands and `results.csv` generation will be added in the next phase; measured data is never fabricated or committed.

## Status

See [PLAN.md](PLAN.md) for requirement mapping, implementation sequence, and remaining data-dependent work.

## Problem A profile

Run `python analysis/problem_a.py creditcard.csv --out results/problem_a` to regenerate the pandas/NumPy profile and labelled histogram. This is the explicitly permitted Python analysis/visualization step; ranking algorithms remain C++. The script makes no silent row drops or imputations and records its standard-deviation convention in the output.
