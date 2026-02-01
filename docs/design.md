
# incident-snapshot-cpp — Design Notes

## Purpose

This tool provides a fast, low-impact snapshot of a Linux system during:
- suspected security incidents
- post-alert triage
- lab-based forensic analysis

---

## Threat Model

The tool assists in identifying indicators related to:
- unauthorized logins
- suspicious processes
- unexpected network listeners
- persistence via services
- authentication anomalies

It does **not** exploit vulnerabilities or modify system state.

---

## Design Goals

- Written in **C++17** using the standard library
- Linux-first, OS-specific by design
- Safe execution without root privileges
- Clear, human-readable output
- Graceful failure in restricted environments

---

## Data Collection Strategy

### Command execution
Commands are executed using `popen()` with stderr redirected to stdout to ensure:
- full visibility of failures
- no silent errors

### Filesystem usage
- Each run creates a timestamped directory
- Output files are overwritten per run directory
- No temporary files are left behind

---

## Handling Restricted Environments

School and lab systems often:
- restrict `/var/log`
- disable systemd
- block certain networking commands

The tool:
- detects missing tools
- records error output instead of failing
- continues execution even if a section fails

---

## Limitations

- No persistence detection
- No kernel-level inspection
- Not a replacement for full forensic suites
- Snapshot is point-in-time only

---

## Future Improvements

- JSON output mode for automation
- Selective module flags (`--network`, `--processes`)
- Hashing of suspicious binaries
- Optional compression of snapshot directory
