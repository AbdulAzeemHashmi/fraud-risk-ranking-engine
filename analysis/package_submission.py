"""Create a curated section submission ZIP without data or build products."""

from __future__ import annotations

import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ARCHIVE = ROOT / "2013-A2-A.zip"
FILES = [
    "README.md", "PLAN.md", "CMakeLists.txt", "requirements.txt", "results.csv", "2013-A2-A.pdf",
    "assets/header.svg",
    "include", "src", "tests",
    "analysis/analyst_console.py", "analysis/problem_a.py",
    "analysis/summarize_results.py", "analysis/build_report.py",
    "analysis/package_submission.py",
    "results/problem_a", "results/experiments_final_v2",
]
SKIP_PARTS = {"build", ".git", "__pycache__", ".pytest_cache"}
SKIP_SUFFIXES = {".exe", ".o", ".obj", ".pdb", ".pyc"}


def main() -> None:
    with zipfile.ZipFile(ARCHIVE, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=8) as zf:
        for item in FILES:
            path = ROOT / item
            if path.is_file():
                zf.write(path, Path("2013-A2-A") / path.relative_to(ROOT))
                continue
            for child in sorted(path.rglob("*")):
                if not child.is_file():
                    continue
                rel = child.relative_to(ROOT)
                if any(part in SKIP_PARTS for part in rel.parts) or child.suffix.lower() in SKIP_SUFFIXES:
                    continue
                zf.write(child, Path("2013-A2-A") / rel)
    with zipfile.ZipFile(ARCHIVE) as zf:
        names = zf.namelist()
        forbidden = [name for name in names if "creditcard.csv" in name.lower() or "/build/" in name.lower() or "/.git/" in name.lower()]
        if forbidden:
            ARCHIVE.unlink(missing_ok=True)
            raise RuntimeError(f"Forbidden package content: {forbidden}")
        print(f"Wrote {ARCHIVE} with {len(names)} files, {ARCHIVE.stat().st_size:,} bytes")


if __name__ == "__main__":
    main()
