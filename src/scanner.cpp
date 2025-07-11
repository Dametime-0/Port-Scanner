#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <thread>
#include <mutex>
#include <string>
#include <atomic>
#include <algorithm>
#include <chrono>
#include <sstream>
#include <fstream>

#pragma comment(lib, "ws2_32.lib")

// 扫描结果结构体
struct ScanResult {
    int port;
    bool isOpen;
    std::string service;
};

// 全局变量
std::mutex resultMutex;             // 保护结果向量的互斥锁
std::vector<ScanResult> results;    // 存储扫描结果
std::atomic<int> scannedPorts(0);   // 已扫描端口计数
std::atomic<bool> verbose(false);   // 详细模式标志

// 解析命令行参数
struct Options {
    std::string target;
    int startPort = 1;
    int endPort = 1000;
    int threads = 50;
    bool verbose = false;
    std::string outputFile;
};

Options parseOptions(int argc, char* argv[]) {
    Options opts;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            std::cout << "Usage: PortScanner [options] <target>\n"
                << "Options:\n"
                << "  -p, --ports <start-end>  Port range (default: 1-1000)\n"
                << "  -t, --threads <num>      Number of threads (default: 50)\n"
                << "  -v, --verbose            Show detailed output\n"
                << "  -o, --output <file>      Output file\n"
                << "  -h, --help               Show this help message\n";
            exit(0);
        }
        else if (arg == "-p" || arg == "--ports") {
            if (i + 1 < argc) {
                std::string portRange = argv[++i];
                size_t dashPos = portRange.find('-');
                if (dashPos != std::string::npos) {
                    opts.startPort = static_cast<int>(std::stoi(portRange.substr(0, dashPos)));
                    opts.endPort = static_cast<int>(std::stoi(portRange.substr(dashPos + 1)));
                }
                else {
                    opts.startPort = opts.endPort = std::stoi(portRange);
                }
            }
        }
        else if (arg == "-t" || arg == "--threads") {
            if (i + 1 < argc) {
                opts.threads = std::stoi(argv[++i]);
            }
        }
        else if (arg == "-v" || arg == "--verbose") {
            opts.verbose = true;
            verbose = true;
        }
        else if (arg == "-o" || arg == "--output") {
            if (i + 1 < argc) {
                opts.outputFile = argv[++i];
            }
        }
        else if (arg[0] != '-') {
            opts.target = arg;
        }
    }

    if (opts.target.empty()) {
        std::cerr << "Error: Target is required.\n";
        exit(1);
    }

    return opts;
}

// 获取服务名称（简单映射）
std::string getServiceName(int port) {
    static const struct {
        int port;
        const char* service;
    } services[] = {
        {21, "FTP"}, {22, "SSH"}, {23, "Telnet"}, {25, "SMTP"},
        {53, "DNS"}, {80, "HTTP"}, {110, "POP3"}, {143, "IMAP"},
        {443, "HTTPS"}, {3306, "MySQL"}, {5432, "PostgreSQL"}
    };

    for (const auto& s : services) {
        if (s.port == port) {
            return s.service;
        }
    }

    return "Unknown";
}

// 获取服务Banner
std::string getBanner(SOCKET sock, int port) {
    char buffer[1024] = { 0 };

    // 根据端口类型发送不同请求
    if (port == 80 || port == 443) {
        const char* request = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
        send(sock, request, strlen(request), 0);
    }
    else if (port == 21) {
        const char* request = "USER anonymous\r\n";
        send(sock, request, strlen(request), 0);
    }
    else if (port == 22) {
        // SSH服务会主动发送banner
    }

    // 接收响应
    int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes > 0) {
        // 简单清理响应内容
        std::string banner(buffer, bytes);
        // 移除换行符和控制字符
        banner.erase(std::remove(banner.begin(), banner.end(), '\r'), banner.end());
        if (banner.length() > 50) {
            banner = banner.substr(0, 50) + "...";
        }
        return banner;
    }

    return "";
}

// 扫描单个端口
bool scanPort(const std::string& target, int port, int timeoutMs = 1000) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        return false;
    }

    // 设置超时
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeoutMs, sizeof(timeoutMs));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeoutMs, sizeof(timeoutMs));

    // 解析目标地址
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    // 尝试解析为IP地址
    if (inet_pton(AF_INET, target.c_str(), &addr.sin_addr) != 1) {
        // 尝试解析为域名
        addrinfo hints = { 0 };
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        addrinfo* result;

        if (getaddrinfo(target.c_str(), nullptr, &hints, &result) != 0) {
            closesocket(sock);
            return false;
        }

        memcpy(&addr, result->ai_addr, sizeof(addr));
        freeaddrinfo(result);
    }

    // 尝试连接
    bool isOpen = (connect(sock, (sockaddr*)&addr, sizeof(addr)) == 0);

    // 获取Banner（如果端口开放）
    if (isOpen && verbose) {
        std::string banner = getBanner(sock, port);
        if (!banner.empty()) {
            std::lock_guard<std::mutex> lock(resultMutex);
            results.push_back({ port, true, getServiceName(port) + " (" + banner + ")" });
        }
        else {
            std::lock_guard<std::mutex> lock(resultMutex);
            results.push_back({ port, true, getServiceName(port) });
        }
    }
    else if (isOpen) {
        std::lock_guard<std::mutex> lock(resultMutex);
        results.push_back({ port, true, getServiceName(port) });
    }

    closesocket(sock);
    scannedPorts++;
    return isOpen;
}

// 扫描端口范围（线程函数）
void scanRange(const std::string& target, int startPort, int endPort, int timeoutMs) {
    for (int port = startPort; port <= endPort; ++port) {
        bool isOpen = scanPort(target, port, timeoutMs);

        if (isOpen) {
            std::lock_guard<std::mutex> lock(resultMutex);
            if (verbose) {
                std::cout << "[+] Port " << port << " is open ("
                    << results.back().service << ")" << std::endl;
            }
        }
        else if (verbose) {
            std::cout << "[-] Port " << port << " is closed" << std::endl;
        }
    }
}

// 显示扫描进度
void showProgress(int totalPorts) {
    auto startTime = std::chrono::high_resolution_clock::now();

    while (scannedPorts < totalPorts) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(
            currentTime - startTime).count();

        if (elapsedSeconds > 0) {
            double speed = (double)scannedPorts / elapsedSeconds;
            int remaining = totalPorts - scannedPorts;
            int eta = (speed > 0) ? (int)(remaining / speed) : 0;

            std::cout << "\rScanning: " << scannedPorts << "/" << totalPorts
                << " ports (" << (int)((double)scannedPorts / totalPorts * 100) << "%)"
                << " | Speed: " << (int)speed << " ports/sec"
                << " | ETA: " << eta << "s     ";
            std::cout.flush();
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "\rScanning: " << totalPorts << "/" << totalPorts
        << " ports (100%)                         " << std::endl;
}

// 保存结果到文件
void saveResults(const std::string& filename) {
    std::ofstream file(filename); // 确保 std::ofstream 类型完整
    if (!file.is_open()) {
        std::cerr << "Failed to open output file: " << filename << std::endl;
        return;
    }

    file << "Port,Status,Service\n";
    for (const auto& result : results) {
        file << result.port << ","
             << (result.isOpen ? "open" : "closed") << ","
             << result.service << "\n";
    }

    file.close();
    std::cout << "Results saved to " << filename << std::endl;
}

int main(int argc, char* argv[]) {
    // 初始化Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    // 解析命令行参数
    Options opts = parseOptions(argc, argv);

    std::cout << "Starting port scan on " << opts.target
        << " (" << opts.startPort << "-" << opts.endPort << ")" << std::endl;

    int totalPorts = opts.endPort - opts.startPort + 1;

    // 创建线程
    std::vector<std::thread> threads;
    int portsPerThread = (totalPorts + opts.threads - 1) / opts.threads;

    // 启动进度显示线程
    std::thread progressThread(showProgress, totalPorts);

    // 启动扫描线程
    for (int i = 0; i < opts.threads; ++i) {
        int start = opts.startPort + i * portsPerThread;
        int end = min(start + portsPerThread - 1, opts.endPort);

        if (start <= end) {
            threads.emplace_back(scanRange, opts.target, start, end, 1000);
        }
    }

    // 等待所有线程完成
    for (auto& t : threads) {
        t.join();
    }

    // 等待进度线程完成
    progressThread.join();

    // 输出结果统计
    std::cout << "\nScan completed: " << results.size() << " open ports found" << std::endl;

    // 按端口号排序结果
    std::sort(results.begin(), results.end(),
        [](const ScanResult& a, const ScanResult& b) {
            return a.port < b.port;
        });

    // 打印开放端口
    if (!results.empty()) {
        std::cout << "\nOpen ports:\n";
        for (const auto& result : results) {
            if (result.isOpen) {
                std::cout << "  Port " << result.port << ": " << result.service << std::endl;
            }
        }
    }

    // 保存结果到文件
    if (!opts.outputFile.empty()) {
        saveResults(opts.outputFile);
    }

    // 清理Winsock
    WSACleanup();
    return 0;
}