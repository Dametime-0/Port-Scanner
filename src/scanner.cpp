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

// ɨ�����ṹ��
struct ScanResult {
    int port;
    bool isOpen;
    std::string service;
};

// ȫ�ֱ���
std::mutex resultMutex;             // ������������Ļ�����
std::vector<ScanResult> results;    // �洢ɨ����
std::atomic<int> scannedPorts(0);   // ��ɨ��˿ڼ���
std::atomic<bool> verbose(false);   // ��ϸģʽ��־

// ���������в���
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

// ��ȡ�������ƣ���ӳ�䣩
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

// ��ȡ����Banner
std::string getBanner(SOCKET sock, int port) {
    char buffer[1024] = { 0 };

    // ���ݶ˿����ͷ��Ͳ�ͬ����
    if (port == 80 || port == 443) {
        const char* request = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
        send(sock, request, strlen(request), 0);
    }
    else if (port == 21) {
        const char* request = "USER anonymous\r\n";
        send(sock, request, strlen(request), 0);
    }
    else if (port == 22) {
        // SSH�������������banner
    }

    // ������Ӧ
    int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes > 0) {
        // ��������Ӧ����
        std::string banner(buffer, bytes);
        // �Ƴ����з��Ϳ����ַ�
        banner.erase(std::remove(banner.begin(), banner.end(), '\r'), banner.end());
        if (banner.length() > 50) {
            banner = banner.substr(0, 50) + "...";
        }
        return banner;
    }

    return "";
}

// ɨ�赥���˿�
bool scanPort(const std::string& target, int port, int timeoutMs = 1000) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        return false;
    }

    // ���ó�ʱ
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeoutMs, sizeof(timeoutMs));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeoutMs, sizeof(timeoutMs));

    // ����Ŀ���ַ
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    // ���Խ���ΪIP��ַ
    if (inet_pton(AF_INET, target.c_str(), &addr.sin_addr) != 1) {
        // ���Խ���Ϊ����
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

    // ��������
    bool isOpen = (connect(sock, (sockaddr*)&addr, sizeof(addr)) == 0);

    // ��ȡBanner������˿ڿ��ţ�
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

// ɨ��˿ڷ�Χ���̺߳�����
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

// ��ʾɨ�����
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

// ���������ļ�
void saveResults(const std::string& filename) {
    std::ofstream file(filename); // ȷ�� std::ofstream ��������
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
    // ��ʼ��Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    // ���������в���
    Options opts = parseOptions(argc, argv);

    std::cout << "Starting port scan on " << opts.target
        << " (" << opts.startPort << "-" << opts.endPort << ")" << std::endl;

    int totalPorts = opts.endPort - opts.startPort + 1;

    // �����߳�
    std::vector<std::thread> threads;
    int portsPerThread = (totalPorts + opts.threads - 1) / opts.threads;

    // ����������ʾ�߳�
    std::thread progressThread(showProgress, totalPorts);

    // ����ɨ���߳�
    for (int i = 0; i < opts.threads; ++i) {
        int start = opts.startPort + i * portsPerThread;
        int end = min(start + portsPerThread - 1, opts.endPort);

        if (start <= end) {
            threads.emplace_back(scanRange, opts.target, start, end, 1000);
        }
    }

    // �ȴ������߳����
    for (auto& t : threads) {
        t.join();
    }

    // �ȴ������߳����
    progressThread.join();

    // ������ͳ��
    std::cout << "\nScan completed: " << results.size() << " open ports found" << std::endl;

    // ���˿ں�������
    std::sort(results.begin(), results.end(),
        [](const ScanResult& a, const ScanResult& b) {
            return a.port < b.port;
        });

    // ��ӡ���Ŷ˿�
    if (!results.empty()) {
        std::cout << "\nOpen ports:\n";
        for (const auto& result : results) {
            if (result.isOpen) {
                std::cout << "  Port " << result.port << ": " << result.service << std::endl;
            }
        }
    }

    // ���������ļ�
    if (!opts.outputFile.empty()) {
        saveResults(opts.outputFile);
    }

    // ����Winsock
    WSACleanup();
    return 0;
}