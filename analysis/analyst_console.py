"""A dependency-free local web page for the C++ top-K queue methods."""

from __future__ import annotations

import argparse
import json
import os
import subprocess
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_DATA = ROOT / "creditcard.csv"
DEFAULT_EXE = ROOT / "build" / "analyst_console.exe"
METHODS = {
    "merge_sort",
    "size_k_min_heap",
    "bounded_insertion_buffer",
}

PAGE = r"""<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>The Midnight Queue | Analyst Console</title>
  <style>
    :root { color-scheme: light; font-family: Segoe UI, Arial, sans-serif; color: #172334; background: #f3f6fa; }
    body { margin: 0; }
    main { max-width: 1100px; margin: 36px auto; padding: 0 20px; }
    h1 { margin-bottom: 6px; color: #12345a; }
    .intro { color: #53657b; margin-top: 0; }
    section, .metric { background: white; border: 1px solid #dce4ed; border-radius: 10px; }
    section { padding: 20px; margin: 18px 0; }
    form { display: flex; flex-wrap: wrap; align-items: end; gap: 14px; }
    label { display: grid; gap: 6px; color: #384d65; font-weight: 600; }
    select, input, button { min-height: 40px; border: 1px solid #b8c8da; border-radius: 6px; padding: 8px 10px; font: inherit; background: white; }
    select { min-width: 280px; } input { width: 120px; }
    button { background: #145da0; border-color: #145da0; color: white; font-weight: 700; cursor: pointer; }
    button:disabled { opacity: .55; cursor: wait; }
    #status { color: #53657b; min-height: 24px; }
    .metrics { display: grid; grid-template-columns: repeat(auto-fit, minmax(170px, 1fr)); gap: 10px; }
    .metric { padding: 13px; }
    .metric span { display: block; color: #64758a; font-size: .88rem; }
    .metric strong { display: block; margin-top: 4px; font-size: 1.15rem; }
    .table-wrap { overflow: auto; max-height: 62vh; }
    table { width: 100%; border-collapse: collapse; background: white; }
    th, td { padding: 10px 12px; border-bottom: 1px solid #e5ebf1; text-align: right; white-space: nowrap; }
    th { position: sticky; top: 0; background: #eaf0f6; color: #31465d; }
    th:first-child, td:first-child { text-align: left; }
    #results[hidden] { display: none; }
    .note { color: #64758a; font-size: .9rem; }
  </style>
</head>
<body>
<main>
  <h1>The Midnight Queue</h1>
  <p class="intro">Choose a queue method and K. The C++ program ranks the full data file and reports the work it performed.</p>
  <section>
    <form id="queue-form">
      <label>Queue method
        <select name="algorithm">
          <option value="merge_sort">Full Sort: Merge Sort, then take K</option>
          <option value="size_k_min_heap">Size-K Min-Heap</option>
          <option value="bounded_insertion_buffer">Bounded Insertion Buffer</option>
        </select>
      </label>
      <label>Queue size, K
        <input name="k" type="number" min="1" max="284807" value="230" required>
      </label>
      <button id="run-button" type="submit">Build queue</button>
    </form>
    <p id="status" role="status">Ready. The Class label is not shown or used to rank transactions.</p>
  </section>
  <section id="results" hidden>
    <div id="metrics" class="metrics"></div>
    <h2>Top transactions</h2>
    <div class="table-wrap">
      <table>
        <thead><tr><th>Rank</th><th>Transaction ID</th><th>Risk band</th><th>Amount</th><th>Time</th></tr></thead>
        <tbody id="transactions"></tbody>
      </table>
    </div>
  </section>
  <p class="note">Times cover queue construction after the data file is loaded. Comparisons count calls to the priority comparator.</p>
</main>
<script>
const form = document.querySelector('#queue-form');
const button = document.querySelector('#run-button');
const statusLine = document.querySelector('#status');
const results = document.querySelector('#results');
const metrics = document.querySelector('#metrics');
const body = document.querySelector('#transactions');
const number = new Intl.NumberFormat(undefined, { maximumFractionDigits: 6 });
form.addEventListener('submit', async (event) => {
  event.preventDefault();
  button.disabled = true;
  results.hidden = true;
  statusLine.textContent = 'Building the queue in C++. Large K values can take longer.';
  try {
    const response = await fetch('/run', { method: 'POST', body: new URLSearchParams(new FormData(form)) });
    const message = await response.json();
    if (!response.ok) throw new Error(message.error || 'The run failed.');
    const data = message.data;
    metrics.replaceChildren();
    const items = [
      ['Method', data.algorithm_name], ['Rows ranked', number.format(data.rows)],
      ['Requested K', number.format(data.requested_k)], ['Queue size', number.format(data.queue_size)],
      ['Running time', `${number.format(data.seconds)} seconds`],
      ['Comparisons', number.format(data.comparisons)], ['Moves', number.format(data.moves)]
    ];
    for (const [label, value] of items) {
      const card = document.createElement('div'); card.className = 'metric';
      const caption = document.createElement('span'); caption.textContent = label;
      const result = document.createElement('strong'); result.textContent = value;
      card.append(caption, result); metrics.append(card);
    }
    body.replaceChildren();
    for (const [index, record] of data.transactions.entries()) {
      const row = document.createElement('tr');
      for (const value of [index + 1, record.id, record.risk_band, number.format(record.amount), number.format(record.time)]) {
        const cell = document.createElement('td'); cell.textContent = value; row.append(cell);
      }
      body.append(row);
    }
    results.hidden = false;
    statusLine.textContent = `Queue built successfully. Showing ${data.queue_size} transactions.`;
  } catch (error) {
    statusLine.textContent = error.message;
  } finally {
    button.disabled = false;
  }
});
</script>
</body>
</html>"""


class ConsoleHandler(BaseHTTPRequestHandler):
    server_version = "MidnightQueueConsole/1.0"

    def _send(self, status: int, content_type: str, body: bytes) -> None:
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def _send_json(self, status: int, payload: dict) -> None:
        body = json.dumps(payload).encode("utf-8")
        self._send(status, "application/json; charset=utf-8", body)

    def do_GET(self) -> None:
        if self.path != "/":
            self._send(404, "text/plain; charset=utf-8", b"Not found")
            return
        self._send(200, "text/html; charset=utf-8", PAGE.encode("utf-8"))

    def do_POST(self) -> None:
        if self.path != "/run":
            self._send_json(404, {"error": "Not found"})
            return
        try:
            length = int(self.headers.get("Content-Length", "0"))
            if length <= 0 or length > 4096:
                raise ValueError("Invalid request size.")
            fields = parse_qs(self.rfile.read(length).decode("utf-8"), strict_parsing=True)
            method = fields.get("algorithm", [""])[0]
            if method not in METHODS:
                raise ValueError("Choose one of the listed queue methods.")
            raw_k = fields.get("k", [""])[0]
            if not raw_k.isdigit() or int(raw_k) < 1:
                raise ValueError("K must be a positive whole number.")
            executable: Path = self.server.executable  # type: ignore[attr-defined]
            data_file: Path = self.server.data_file  # type: ignore[attr-defined]
            if not executable.is_file():
                raise RuntimeError("The C++ console executable is missing. Compile it using the README command.")
            if not data_file.is_file():
                raise RuntimeError(f"Dataset file not found: {data_file}")
            environment = os.environ.copy()
            mingw_bin = Path(r"C:\msys64\mingw64\bin")
            if mingw_bin.is_dir():
                environment["PATH"] = str(mingw_bin) + os.pathsep + environment.get("PATH", "")
            completed = subprocess.run(
                [str(executable), str(data_file), method, raw_k],
                cwd=ROOT,
                capture_output=True,
                text=True,
                timeout=120,
                check=False,
                env=environment,
            )
            if completed.returncode != 0:
                message = completed.stderr.strip() or f"The C++ queue run failed with status {completed.returncode}."
                self._send_json(400, {"error": message})
                return
            result = json.loads(completed.stdout)
            result["algorithm_name"] = {
                "merge_sort": "Full Sort: Merge Sort",
                "size_k_min_heap": "Size-K Min-Heap",
                "bounded_insertion_buffer": "Bounded Insertion Buffer",
            }[method]
            self._send_json(200, {"data": result})
        except (UnicodeDecodeError, ValueError) as error:
            self._send_json(400, {"error": str(error)})
        except subprocess.TimeoutExpired:
            self._send_json(408, {"error": "This run took more than 120 seconds. Try a smaller K or a faster queue method."})
        except (OSError, json.JSONDecodeError, RuntimeError) as error:
            self._send_json(500, {"error": str(error)})

    def log_message(self, fmt: str, *args: object) -> None:
        print(f"{self.address_string()} - {fmt % args}")


class ConsoleServer(ThreadingHTTPServer):
    executable: Path
    data_file: Path


def main() -> None:
    parser = argparse.ArgumentParser(description="Run the local Midnight Queue analyst console.")
    parser.add_argument("--data", type=Path, default=DEFAULT_DATA, help="Path to creditcard.csv")
    parser.add_argument("--exe", type=Path, default=DEFAULT_EXE, help="Path to the compiled C++ console")
    parser.add_argument("--port", type=int, default=8000, help="Local web server port")
    args = parser.parse_args()
    data_file = args.data.resolve()
    executable = args.exe.resolve()
    if not 1 <= args.port <= 65535:
        parser.error("port must be between 1 and 65535")

    server = ConsoleServer(("127.0.0.1", args.port), ConsoleHandler)
    server.data_file = data_file
    server.executable = executable
    print(f"Analyst Console: http://127.0.0.1:{args.port}")
    print("Press Ctrl+C to stop the local server.")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("Stopping Analyst Console.")
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
