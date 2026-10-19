# The Midnight Queue — C++ Assignment 2

C++ implementation and experiments for the Design and Analysis of Algorithms assignment. The PDF requirements are treated as the specification; all executable code is C++ as requested.

## Personal parameters

- Roll number / random seed: `2013`
- Shift capacity: `K = 25 + (2013 mod 226) = 46`
- Dataset: obtain `creditcard.csv` from the Kaggle page in the assignment. Do not commit or include the dataset.

## Build

Requires a C++17 compiler and CMake 3.16 or newer.

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

The experiment runner will accept the dataset path as a command-line argument. Results are generated locally as `results.csv`; measured data is never fabricated or committed.

## Status

See [PLAN.md](PLAN.md) for requirement mapping, implementation sequence, and remaining data-dependent work.
