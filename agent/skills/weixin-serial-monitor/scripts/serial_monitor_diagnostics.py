#!/usr/bin/env python3
"""Run safe, read-oriented WEIXIN Serial Monitor diagnostics for one project."""

from __future__ import annotations

import argparse
import json
import math
import subprocess
import time
import uuid
from pathlib import Path
from typing import Any


EXE_NAME = "SerialMonitor.exe"
RUNTIME_FILES = ("ai_commands.jsonl", "ai_results.jsonl", "ai_wave_summary.json")
SKIP_DIRECTORIES = {".git", ".venv", "node_modules", "__pycache__"}
PROTOCOL_REPORT_PATTERN = "protocol_check_*.json"


def has_runtime_files(directory: Path) -> bool:
    return any((directory / name).exists() for name in RUNTIME_FILES)


def runtime_dir(root: Path, explicit: str = "") -> Path:
    if explicit:
        return Path(explicit).expanduser().resolve()
    source_dir = root / "Serial_Monitor"
    source_runtime = source_dir / "runtime"
    if source_runtime.exists() or (source_dir / "src" / "serial_monitor" / "monitor_qt.py").exists():
        return source_runtime
    if (root / EXE_NAME).is_file():
        return root / "runtime"
    if root.name.lower() == "runtime" or has_runtime_files(root):
        return root
    return root / "agent" / "serial"


def discover_executables(root: Path, max_depth: int = 3) -> list[Path]:
    found: set[Path] = set()
    pending: list[tuple[Path, int]] = [(root, 0)]
    while pending:
        directory, depth = pending.pop(0)
        candidate = directory / EXE_NAME
        if candidate.is_file():
            found.add(candidate.resolve())
        if depth >= max_depth or not directory.is_dir():
            continue
        try:
            children = list(directory.iterdir())
        except OSError:
            continue
        pending.extend(
            (child, depth + 1)
            for child in children
            if child.is_dir() and child.name.lower() not in SKIP_DIRECTORIES
        )
    return sorted(found, key=lambda item: str(item).lower())


def resolve_executable(root: Path, supplied: str) -> Path:
    if supplied:
        executable = Path(supplied).expanduser().resolve()
        if executable.is_file():
            return executable
        raise SystemExit(f"Serial Monitor executable does not exist: {executable}")
    candidates = discover_executables(root)
    if not candidates:
        raise SystemExit(f"No {EXE_NAME} found under {root}; pass launch --exe <path>.")
    if len(candidates) > 1:
        choices = "\n".join(f"- {candidate}" for candidate in candidates)
        raise SystemExit(f"Multiple {EXE_NAME} candidates found; pass launch --exe <path>:\n{choices}")
    return candidates[0]


def bridge_paths(runtime: Path) -> tuple[Path, Path, Path]:
    return (
        runtime / "ai_commands.jsonl",
        runtime / "ai_results.jsonl",
        runtime / "ai_wave_summary.json",
    )


def append_jsonl(path: Path, item: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("a", encoding="utf-8") as stream:
        stream.write(json.dumps(item, ensure_ascii=False, separators=(",", ":")))
        stream.write("\n")


def read_results(path: Path, offset: int) -> tuple[int, list[dict[str, Any]]]:
    if not path.exists():
        return offset, []
    size = path.stat().st_size
    if offset < 0 or offset > size:
        offset = 0
    rows: list[dict[str, Any]] = []
    with path.open("rb") as stream:
        stream.seek(offset)
        while True:
            line_offset = stream.tell()
            line = stream.readline()
            if not line:
                break
            if not line.endswith(b"\n"):
                stream.seek(line_offset)
                break
            offset = stream.tell()
            try:
                text = line.decode("utf-8")
            except UnicodeDecodeError:
                continue
            if not text.strip():
                continue
            try:
                row = json.loads(text)
            except json.JSONDecodeError:
                continue
            if isinstance(row, dict):
                rows.append(row)
        offset = stream.tell()
    return offset, rows


def read_json_object(path: Path) -> tuple[dict[str, Any] | None, str]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError:
        return None, "not_found"
    except json.JSONDecodeError:
        return None, "invalid_json"
    except (OSError, UnicodeError):
        return None, "unreadable"
    if not isinstance(value, dict):
        return None, "not_an_object"
    return value, ""


def read_summary(path: Path, max_age: float) -> dict[str, Any]:
    value, error = read_json_object(path)
    if value is None:
        return {"ok": False, "error": error, "message": f"summary {error}", "path": str(path)}
    updated_at = value.get("updated_at")
    if (
        not isinstance(updated_at, (int, float))
        or isinstance(updated_at, bool)
        or not math.isfinite(float(updated_at))
    ):
        return {
            "ok": False,
            "error": "missing_updated_at",
            "message": "summary has no numeric updated_at",
            "path": str(path),
        }
    age = max(0.0, time.time() - float(updated_at))
    if age > max_age:
        return {
            "ok": False,
            "error": "stale",
            "message": f"summary is stale ({age:.3f}s old)",
            "path": str(path),
            "age_seconds": age,
            "max_age_seconds": max_age,
            "data": value,
        }
    return {
        "ok": True,
        "path": str(path),
        "age_seconds": age,
        "max_age_seconds": max_age,
        "data": value,
    }


def latest_protocol_report(runtime: Path, after: float = 0.0) -> dict[str, Any]:
    report_dir = runtime / "protocol_reports"
    try:
        candidates = sorted(
            report_dir.glob(PROTOCOL_REPORT_PATTERN),
            key=lambda candidate: (candidate.stat().st_mtime, candidate.name),
            reverse=True,
        )
    except OSError as exc:
        return {"ok": False, "error": "unreadable", "message": str(exc), "path": str(report_dir)}
    for candidate in candidates:
        try:
            modified_at = candidate.stat().st_mtime
        except OSError:
            continue
        if modified_at <= after:
            continue
        value, error = read_json_object(candidate)
        if value is None:
            continue
        return {
            "ok": True,
            "path": str(candidate),
            "modified_at": modified_at,
            "data": value,
        }
    return {
        "ok": False,
        "error": "not_found",
        "message": "no complete protocol report found",
        "path": str(report_dir),
        "after": after,
    }


def enqueue(runtime: Path, command: dict[str, Any], wait: bool, timeout: float) -> dict[str, Any]:
    command_path, result_path, _summary_path = bridge_paths(runtime)
    command.setdefault("id", str(uuid.uuid4()))
    command_id = str(command["id"])
    try:
        offset = result_path.stat().st_size if result_path.exists() else 0
        append_jsonl(command_path, command)
    except OSError as exc:
        return {
            "id": command_id,
            "ok": False,
            "error": "queue_io_error",
            "message": str(exc),
            "path": str(command_path),
        }
    if not wait:
        return {"id": command_id, "ok": True, "message": "queued", "command": command}

    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            offset, rows = read_results(result_path, offset)
        except OSError as exc:
            return {
                "id": command_id,
                "ok": False,
                "error": "result_io_error",
                "message": str(exc),
                "path": str(result_path),
            }
        for row in rows:
            if str(row.get("id") or "") == command_id:
                return row
        time.sleep(0.1)
    return {"id": command_id, "ok": False, "message": "timeout waiting for ai_results.jsonl"}


def wait_for_monitor(runtime: Path, timeout: float) -> dict[str, Any]:
    deadline = time.monotonic() + timeout
    latest: dict[str, Any] = {"ok": False, "message": "monitor did not become ready"}
    while time.monotonic() < deadline:
        latest = enqueue(runtime, {"type": "gui", "action": "status"}, True, min(2.0, max(0.5, deadline - time.monotonic())))
        if latest.get("ok"):
            return latest
        time.sleep(0.2)
    return latest


def wait_for_protocol_report(runtime: Path, timeout: float, after: float = 0.0) -> dict[str, Any]:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        result = latest_protocol_report(runtime, after)
        if result.get("ok"):
            return result
        time.sleep(0.1)
    return {
        "ok": False,
        "error": "timeout",
        "message": "timeout waiting for a complete protocol report",
        "path": str(runtime / "protocol_reports"),
        "after": after,
    }


def wait_for_scan(
    runtime: Path,
    timeout: float,
    minimum_count: int = 1,
    stable_polls: int = 2,
) -> dict[str, Any]:
    deadline = time.monotonic() + timeout
    last_count: int | None = None
    stable_count = 0
    latest: dict[str, Any] = {}
    while time.monotonic() < deadline:
        latest = enqueue(
            runtime,
            {"type": "gui", "action": "status"},
            True,
            min(2.0, max(0.1, deadline - time.monotonic())),
        )
        data = latest.get("data")
        if not latest.get("ok") or not isinstance(data, dict):
            time.sleep(0.1)
            continue
        if data.get("connected") is False:
            return {"ok": False, "error": "disconnected", "message": "serial disconnected", "status": latest}
        count = data.get("param_count")
        if not isinstance(count, int) or count < minimum_count:
            last_count = count if isinstance(count, int) else None
            stable_count = 0
            time.sleep(0.1)
            continue
        stable_count = stable_count + 1 if count == last_count else 1
        last_count = count
        if stable_count >= stable_polls:
            return {"ok": True, "message": "parameter scan settled", "param_count": count, "status": latest}
        time.sleep(0.1)
    return {
        "ok": False,
        "error": "timeout",
        "message": "timeout waiting for parameter scan to settle",
        "minimum_count": minimum_count,
        "last_status": latest,
    }


def print_json(value: Any) -> None:
    print(json.dumps(value, ensure_ascii=False, indent=2))


def positive_float(value: str) -> float:
    number = float(value)
    if not math.isfinite(number) or number <= 0:
        raise argparse.ArgumentTypeError("must be a finite number greater than zero")
    return number


def nonnegative_float(value: str) -> float:
    number = float(value)
    if not math.isfinite(number) or number < 0:
        raise argparse.ArgumentTypeError("must be a finite non-negative number")
    return number


def positive_int(value: str) -> int:
    number = int(value)
    if number <= 0:
        raise argparse.ArgumentTypeError("must be greater than zero")
    return number


def bounded_log_lines(value: str) -> int:
    number = int(value)
    if not 1 <= number <= 500:
        raise argparse.ArgumentTypeError("must be between 1 and 500")
    return number


def add_log_arguments(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--lines", type=bounded_log_lines, default=200)
    parser.add_argument("--direction", choices=("all", "rx", "tx", "ai", "system"), default="all")
    parser.add_argument("--contains", default="")


def main() -> int:
    parser = argparse.ArgumentParser(description="Run safe WEIXIN Serial Monitor diagnostics for one project runtime.")
    parser.add_argument("--root", default=".", help="Project root, monitor source root, packaged app directory, or runtime directory.")
    parser.add_argument("--runtime", default="", help="Explicit runtime directory.")
    parser.add_argument("--no-wait", action="store_true", help="Queue a command without waiting for its result.")
    parser.add_argument("--timeout", type=positive_float, default=5.0)
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("discover")
    sub.add_parser("status")
    sub.add_parser("refresh-ports")

    launch = sub.add_parser("launch")
    launch.add_argument("--exe", default="")
    launch.add_argument("--port", default="")
    launch.add_argument("--baud", type=positive_int)
    launch.add_argument("--wait", action="store_true")

    connect = sub.add_parser("connect")
    connect.add_argument("--port", required=True)
    connect.add_argument("--port-hint", default="")
    connect.add_argument("--baud", type=positive_int, required=True)

    sub.add_parser("disconnect")
    scan = sub.add_parser("scan-params")
    scan.add_argument("--wait-completion", action="store_true")
    protocol_check = sub.add_parser("protocol-check")
    protocol_check.add_argument("--wait-completion", action="store_true")
    sub.add_parser("latest-protocol-report")
    wait_report = sub.add_parser("wait-protocol-report")
    wait_report.add_argument("--after", type=nonnegative_float, default=0.0)
    wait_scan = sub.add_parser("wait-scan")
    wait_scan.add_argument("--minimum-count", type=positive_int, default=1)
    wait_scan.add_argument("--stable-polls", type=positive_int, default=2)
    param_get = sub.add_parser("param-get")
    param_get.add_argument("name")
    read_log = sub.add_parser("read-log")
    add_log_arguments(read_log)
    summary = sub.add_parser("summary")
    summary.add_argument("--max-age", type=positive_float, default=5.0)

    args = parser.parse_args()
    root = Path(args.root).expanduser().resolve()
    runtime = runtime_dir(root, args.runtime)

    if args.command == "discover":
        print_json(
            {
                "ok": True,
                "project_root": str(root),
                "runtime_dir": str(runtime),
                "runtime_exists": runtime.exists(),
                "runtime_has_bridge_files": has_runtime_files(runtime),
                "executables": [str(candidate) for candidate in discover_executables(root)],
            }
        )
        return 0

    _command_path, _result_path, summary_path = bridge_paths(runtime)
    if args.command == "summary":
        result = read_summary(summary_path, args.max_age)
        print_json(result)
        return 0 if result.get("ok") else 1

    if args.command == "latest-protocol-report":
        result = latest_protocol_report(runtime)
        print_json(result)
        return 0 if result.get("ok") else 1

    if args.command == "wait-protocol-report":
        result = wait_for_protocol_report(runtime, args.timeout, args.after)
        print_json(result)
        return 0 if result.get("ok") else 1

    if args.command == "wait-scan":
        result = wait_for_scan(runtime, args.timeout, args.minimum_count, args.stable_polls)
        print_json(result)
        return 0 if result.get("ok") else 1

    if args.command == "launch":
        executable = resolve_executable(root, args.exe)
        command_line = [str(executable), "--runtime-dir", str(runtime)]
        if args.port:
            command_line.extend(["--port", args.port])
        if args.baud:
            command_line.extend(["--baud", str(args.baud)])
        try:
            subprocess.Popen(command_line, cwd=str(executable.parent))
        except OSError as exc:
            print_json(
                {
                    "ok": False,
                    "error": "launch_failed",
                    "message": str(exc),
                    "exe": str(executable),
                    "runtime_dir": str(runtime),
                }
            )
            return 1
        result: dict[str, Any] = {"ok": True, "message": "launched", "exe": str(executable), "runtime_dir": str(runtime)}
        if args.wait:
            result["status"] = wait_for_monitor(runtime, args.timeout)
            result["ok"] = bool(result["status"].get("ok"))
            if not result["ok"]:
                result["message"] = "launched but did not receive a status reply"
        print_json(result)
        return 0 if result["ok"] else 1

    action_map = {
        "status": "status",
        "refresh-ports": "refresh_ports",
        "disconnect": "disconnect",
        "scan-params": "scan_params",
        "protocol-check": "protocol_check",
    }
    if args.command == "connect":
        if not args.port.strip():
            parser.error("connect --port must not be blank")
        if args.port.strip().lower() == "auto" and not args.port_hint.strip():
            parser.error("connect --port auto requires --port-hint")
        command: dict[str, Any] = {"type": "gui", "action": "connect", "port": args.port, "baud": args.baud}
        if args.port_hint:
            command["port_hint"] = args.port_hint
    elif args.command == "param-get":
        command = {"type": "gui", "action": "param_get", "name": args.name}
    elif args.command == "read-log":
        command = {"type": "gui", "action": "read_serial_log", "lines": args.lines, "direction": args.direction}
        if args.contains:
            command["contains"] = args.contains
    else:
        command = {"type": "gui", "action": action_map[args.command]}

    completion_started_at = time.time()
    result = enqueue(runtime, command, wait=not args.no_wait, timeout=args.timeout)
    if result.get("ok") and getattr(args, "wait_completion", False):
        if args.command == "protocol-check":
            result = wait_for_protocol_report(runtime, args.timeout, completion_started_at)
        elif args.command == "scan-params":
            result = wait_for_scan(runtime, args.timeout)
    print_json(result)
    return 0 if result.get("ok", False) else 1


if __name__ == "__main__":
    raise SystemExit(main())
