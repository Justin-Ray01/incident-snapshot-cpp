#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// Run a shell command and capture stdout+stderr (best-effort).
// Uses popen with "2>&1" to capture stderr too.
static std::string run_capture(const std::string& cmd) {
    std::string full = cmd + " 2>&1";
    std::array<char, 4096> buf{};
    std::string out;

    FILE* pipe = popen(full.c_str(), "r");
    if (!pipe) return "ERROR: popen() failed (command may be restricted)\n";

    while (true) {
        size_t n = fread(buf.data(), 1, buf.size(), pipe);
        if (n > 0) out.append(buf.data(), n);
        if (n < buf.size()) break;
    }

    int rc = pclose(pipe);
    (void)rc; 
    return out;
}

static void write_section(const fs::path& file, const std::string& title, const std::string& body) {
    std::ofstream f(file, std::ios::app);
    f << title << "\n";
    f << std::string(title.size(), '=') << "\n";
    if (body.empty()) f << "(no output)\n\n";
    else {
        f << body;
        if (!body.empty() && body.back() != '\n') f << "\n";
        f << "\n";
    }
}

static std::string now_stamp() {
    using namespace std::chrono;
    auto t = system_clock::now();
    std::time_t tt = system_clock::to_time_t(t);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d_%02d-%02d-%02d",
                  tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                  tm.tm_hour, tm.tm_min, tm.tm_sec);
    return std::string(buf);
}

static void ensure_clean_file(const fs::path& p) {
    std::ofstream f(p, std::ios::trunc);
}

int main() {
    // Output directory
    fs::path out_dir = fs::current_path() / ("incident-snapshot-" + now_stamp());
    std::error_code ec;
    fs::create_directories(out_dir, ec);
    if (ec) {
        std::cerr << "Failed to create output directory: " << out_dir << "\n";
        std::cerr << "Error: " << ec.message() << "\n";
        return 1;
    }

    // Files
    fs::path system_txt   = out_dir / "system.txt";
    fs::path users_txt    = out_dir / "users.txt";
    fs::path proc_txt     = out_dir / "processes.txt";
    fs::path net_txt      = out_dir / "network.txt";
    fs::path services_txt = out_dir / "services.txt";
    fs::path auth_txt     = out_dir / "auth_hints.txt";

    ensure_clean_file(system_txt);
    ensure_clean_file(users_txt);
    ensure_clean_file(proc_txt);
    ensure_clean_file(net_txt);
    ensure_clean_file(services_txt);
    ensure_clean_file(auth_txt);

    // SYSTEM
    write_section(system_txt, "Date",      run_capture("date"));
    write_section(system_txt, "Hostname",  run_capture("hostname"));
    write_section(system_txt, "Uptime",    run_capture("uptime"));
    write_section(system_txt, "Kernel/OS", run_capture("uname -a"));

    // USERS
    write_section(users_txt, "Logged in users (who)", run_capture("who"));
    write_section(users_txt, "Recent logins (last -n 20)", run_capture("last -n 20"));
    write_section(users_txt, "Current user (id)", run_capture("id"));

    // PROCESSES
    write_section(proc_txt, "Top CPU processes (ps aux --sort=-%cpu | head -n 25)",
                  run_capture("ps aux --sort=-%cpu | head -n 25"));
    write_section(proc_txt, "Top MEM processes (ps aux --sort=-%mem | head -n 25)",
                  run_capture("ps aux --sort=-%mem | head -n 25"));

    // NETWORK (prefer ss, fallback netstat)
    std::string has_ss = run_capture("command -v ss");
    if (!has_ss.empty() && has_ss.find("not found") == std::string::npos) {
        write_section(net_txt, "Listening sockets (ss -lntup)", run_capture("ss -lntup"));
        write_section(net_txt, "Active connections (ss -ntup)", run_capture("ss -ntup"));
    } else {
        write_section(net_txt, "Listening sockets (netstat -lntup)", run_capture("netstat -lntup"));
        write_section(net_txt, "Active connections (netstat -ntup)", run_capture("netstat -ntup"));
    }

    // SERVICES (systemd may not exist)
    std::string has_systemctl = run_capture("command -v systemctl");
    if (!has_systemctl.empty() && has_systemctl.find("not found") == std::string::npos) {
        write_section(services_txt, "Running services (systemctl list-units --type=service --state=running)",
                      run_capture("systemctl list-units --type=service --state=running --no-pager"));
    } else {
        write_section(services_txt, "Running services",
                      "systemctl not available on this system.\n");
    }

    // AUTH HINTS (best-effort: auth.log or secure)
    bool any_auth = false;
    if (fs::exists("/var/log/auth.log", ec) && !ec) {
        any_auth = true;
        write_section(auth_txt, "Last 50 lines of /var/log/auth.log", run_capture("tail -n 50 /var/log/auth.log"));
    }
    ec.clear();
    if (fs::exists("/var/log/secure", ec) && !ec) {
        any_auth = true;
        write_section(auth_txt, "Last 50 lines of /var/log/secure", run_capture("tail -n 50 /var/log/secure"));
    }
    if (!any_auth) {
        write_section(auth_txt, "Auth logs",
                      "No auth log found at /var/log/auth.log or /var/log/secure (or access is restricted).\n");
    }

    std::cout << "Incident snapshot saved to:\n  " << out_dir.string() << "\n";
    std::cout << "Files created:\n";
    std::cout << "  - " << system_txt.filename().string() << "\n";
    std::cout << "  - " << users_txt.filename().string() << "\n";
    std::cout << "  - " << proc_txt.filename().string() << "\n";
    std::cout << "  - " << net_txt.filename().string() << "\n";
    std::cout << "  - " << services_txt.filename().string() << "\n";
    std::cout << "  - " << auth_txt.filename().string() << "\n\n";
    std::cout << "Tip: upload a sample run (folder contents) to GitHub under sample-output/.\n";

    return 0;
}
