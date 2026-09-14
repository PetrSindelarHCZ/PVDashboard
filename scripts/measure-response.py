"""Sequential HTTP latency samples, with optional timestamped serial capture."""
import argparse
import csv
import json
import math
from pathlib import Path
import statistics
import threading
import time
from datetime import datetime, timezone
import urllib.request


def utc():
    return datetime.now(timezone.utc).isoformat()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--url", required=True, help="Device base URL")
    parser.add_argument("--duration", type=float, default=300)
    parser.add_argument("--interval", type=float, default=1, help="Pause after each response")
    parser.add_argument("--timeout", type=float, default=15)
    parser.add_argument("--serial", help="Optional COM port; requires pyserial")
    parser.add_argument("--output", default=".pio/measurements")
    args = parser.parse_args()
    if args.duration <= 0 or args.interval < 0 or args.timeout <= 0:
        parser.error("duration/timeout must be positive and interval nonnegative")
    folder = Path(args.output) / datetime.now().strftime("%Y%m%d-%H%M%S-%f")
    folder.mkdir(parents=True)
    stop = threading.Event()
    port = None
    reader = None
    if args.serial:
        import serial
        port = serial.Serial()
        port.port, port.baudrate, port.timeout = args.serial, 115200, 0.5
        port.dtr = port.rts = False
        port.open()

        def capture():
            with (folder / "serial.log").open("w", encoding="utf-8") as log:
                while not stop.is_set():
                    line = port.readline().decode("utf-8", errors="replace").strip()
                    if line:
                        log.write(f"{utc()} {line}\n")
                        log.flush()

        reader = threading.Thread(target=capture)
        reader.start()
    opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))
    successes = []
    attempts = errors = 0
    print(f"Capturing to {folder.resolve()}", flush=True)
    end = time.monotonic() + args.duration
    try:
        with (folder / "http.csv").open("w", newline="", encoding="utf-8") as csv_file, \
                (folder / "status.jsonl").open("w", encoding="utf-8") as snapshots:
            writer = csv.DictWriter(csv_file, fieldnames=["utc", "elapsedMs", "httpStatus", "error"])
            writer.writeheader()
            while time.monotonic() < end:
                stamp, start = utc(), time.perf_counter()
                code, error, data = 0, "", None
                try:
                    request = urllib.request.Request(args.url.rstrip("/") + "/api/status",
                        headers={"Connection": "close", "Cache-Control": "no-cache"})
                    with opener.open(request, timeout=args.timeout) as response:
                        code = response.status
                        payload = response.read()
                    elapsed = (time.perf_counter() - start) * 1000
                    data = json.loads(payload)
                except Exception as exc:
                    elapsed = (time.perf_counter() - start) * 1000
                    code = getattr(exc, "code", code)
                    error = str(exc)
                    errors += 1
                else:
                    successes.append(elapsed)
                attempts += 1
                writer.writerow(dict(utc=stamp, elapsedMs=round(elapsed, 2), httpStatus=code, error=error))
                csv_file.flush()
                if data is not None:
                    snapshots.write(json.dumps(dict(utc=stamp, elapsedMs=elapsed, status=data)) + "\n")
                    snapshots.flush()
                print(f"{stamp} {elapsed:.0f} ms HTTP {code} {error}", flush=True)
                time.sleep(max(0, min(args.interval, end - time.monotonic())))
    except KeyboardInterrupt:
        print("Stopped; saving partial summary.")
    finally:
        stop.set()
        if reader:
            reader.join()
            port.close()
    ordered = sorted(successes)
    summary = dict(url=args.url, attempts=attempts, errors=errors, successful=len(ordered),
        medianMs=statistics.median(ordered) if ordered else None,
        p95Ms=ordered[math.ceil(len(ordered) * .95) - 1] if ordered else None,
        maxMs=max(ordered) if ordered else None)
    (folder / "summary.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
