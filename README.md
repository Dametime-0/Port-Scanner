# Port-Scanner
Under testing, based on C++
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
```bash
# Scan target with default settings (ports 1-1000, 50 threads)
src/port_scanner.exe <target_ip_or_domain>
