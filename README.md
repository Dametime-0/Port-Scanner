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

# Basic Command
```bash
# Scan target with default settings (ports 1-1000, 50 threads)
src/port_scanner.exe <target_ip_or_domain>

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
g++ src/port_scanner.cpp -o port_scanner.exe -lws2_32

Build with Visual Studio
Create a new "Empty Project"
Add src/port_scanner.cpp to the project
Build the project (ensure "x64" or "x86" configuration matches your system)

Notes
Legal Compliance: Only scan networks or devices you own or have explicit permission to test. Unauthorized scanning may violate laws.
Firewalls: Some ports may be blocked by firewalls, resulting in "filtered" status (not shown as open/closed).
Performance: Adjust thread count based on your system capabilities (too many threads may cause network congestion).


##使用操作

一、准备工作
获取可执行文件
如果你已编译好程序，会得到 port_scanner.exe（通常在 src 目录或编译输出目录）。
若未编译，参考项目中的 Compilation 部分，用 MinGW 或 Visual Studio 编译源码生成 .exe 文件。
打开命令行
按下 Win + R，输入 cmd 并回车，打开 “命令提示符”。
用 cd 命令切换到 port_scanner.exe 所在的目录（例如：cd D:\windows-port-scanner\src）。
二、基本使用方法
1. 简单扫描（默认设置）
扫描目标的 1-1000 端口（默认 50 线程）：

cmd
port_scanner.exe 目标IP或域名

示例：

cmd
port_scanner.exe 127.0.0.1  # 扫描本地回环地址
port_scanner.exe localhost   # 扫描本地主机（等价于127.0.0.1）
port_scanner.exe example.com # 扫描远程域名
2. 指定端口范围
用 -p 参数自定义扫描的端口范围（格式：起始端口-结束端口）：

cmd
port_scanner.exe -p 80-443 目标  # 只扫描80-443端口（常见Web端口）
port_scanner.exe -p 1-65535 目标  # 扫描所有端口（耗时较长）
3. 调整线程数
用 -t 参数设置扫描线程数（线程越多速度越快，但需根据系统性能调整）：

cmd
port_scanner.exe -t 100 目标  # 用100线程扫描（默认50线程）
4. 详细模式（显示关闭的端口）
用 -v 参数开启详细输出，会显示每个端口的关闭状态：

cmd
port_scanner.exe -v 目标  # 显示开放和关闭的端口详情
5. 保存结果到 CSV 文件
用 -o 参数将扫描结果保存为 CSV（可用 Excel 打开）：

cmd
port_scanner.exe -o 结果文件.csv 目标

示例：

cmd
port_scanner.exe -o scan_result.csv 192.168.1.1  # 保存扫描结果到CSV
三、组合使用示例
同时指定端口范围、线程数、详细模式和结果保存：

cmd
port_scanner.exe -p 20-300 -t 80 -v -o result.csv 192.168.1.1

含义：

-p 20-300：扫描 20 到 300 端口
-t 80：使用 80 线程
-v：显示详细输出
-o result.csv：结果保存到 result.csv
目标为 192.168.1.1（局域网内某设备）
四、查看帮助
若忘记参数用法，可输入以下命令查看所有选项：

cmd
port_scanner.exe -h

会显示支持的参数列表和说明。
五、结果说明
扫描完成后，会在命令行显示：

开放的端口号及对应的服务（如 Port 80: HTTP）。
扫描统计（总端口数、开放端口数）。
若使用 -o 参数，CSV 文件会包含端口号、状态（open/closed）、服务名称。
注意事项
扫描他人网络需获得授权，避免触犯法律。
线程数并非越多越好（过多可能导致目标防火墙拦截或本地资源耗尽）。
防火墙可能导致部分端口显示为 “过滤”（不显示开放 / 关闭），属正常现象。

按照以上步骤，即可灵活使用该端口扫描器完成不同场景的端口探测。
