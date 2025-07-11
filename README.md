# Windows Port Scanner

A simple TCP port scanner for Windows, implemented in C++. Supports multi-threaded scanning, service detection, and result export to CSV.


## Features
- Multi-threaded scanning for faster performance
- Service identification based on common port mappings
- Banner grabbing for detailed service information (e.g., HTTP server version)
- Real-time progress tracking (percentage, speed, ETA)
- Command-line interface with configurable options
- Export results to CSV file


## Usage

### Basic Command
# Scan target with default settings (ports 1-1000, 50 threads)
src/port_scanner.exe <target_ip_or_domain>







Advanced Options

# Scan specific port range
src/port_scanner.exe -p 1-65535 <target>

# Set number of threads
src/port_scanner.exe -t 100 <target>

# Enable verbose output (show closed ports)
src/port_scanner.exe -v <target>

# Save results to CSV
src/port_scanner.exe -o results.csv <target>

# Combine options
src/port_scanner.exe -p 80-443 -t 20 -v -o scan_results.csv example.com
Options List
Option	Description
-p, --ports	Port range (format: start-end, default: 1-1000)
-t, --threads	Number of threads (default: 50)
-v, --verbose	Show detailed output (including closed ports)
-o, --output	Path to save CSV results
-h, --help	Show help message
Compilation
Prerequisites
Windows 10/11
Visual Studio (2019+) or MinGW
Winsock2 library (included with Windows SDK)
Build with MinGW

g++ src/port_scanner.cpp -o port_scanner.exe -lws2_32
Build with Visual Studio
Create a new "Empty Project"
Add src/port_scanner.cpp to the project
Build the project (ensure "x64" or "x86" configuration matches your system)
Notes
Legal Compliance: Only scan networks or devices you own or have explicit permission to test. Unauthorized scanning may violate laws.
Firewalls: Some ports may be blocked by firewalls, resulting in "filtered" status (not shown as open/closed).
Performance: Adjust thread count based on your system capabilities (too many threads may cause network congestion).
