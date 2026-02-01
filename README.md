# incident-snapshot-cpp

A **Linux incident-response snapshot tool written in C++17**.

This tool collects a point-in-time snapshot of a Linux system to assist with **incident triage**, **forensics**, and **security investigations**.  
It is designed to run safely in restricted environments (student labs, VMs) without requiring root access.

---

## What it collects

Each run creates a timestamped directory containing:

- `system.txt` – system time, hostname, uptime, kernel info
- `users.txt` – logged-in users, recent logins, current user
- `processes.txt` – top CPU and memory-consuming processes
- `network.txt` – listening sockets and active connections
- `services.txt` – running services (systemd if available)
- `auth_hints.txt` – recent authentication events (best-effort)

Example output directory:

<img width="932" height="624" alt="image" src="https://github.com/user-attachments/assets/5438146e-2713-431f-96db-22969f870ce9" />


---

## Build & Run (Linux)

### Requirements
- Linux system
- `g++` with C++17 support

Install compiler if needed:
```bash
sudo apt update
sudo apt install g++

g++ -std=c++17 -O2 -Wall -Wextra -o incident_snapshot main.cpp

./incident_snapshot
