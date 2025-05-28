#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlwapi.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <fstream>
#include <thread>
#include <string>
#include <map>
#include <ctime>
#include <vector>
#include <sstream>
#include <array>
#include <cstring>
#include <stdint.h>
#include "sha1.h"
#include <cctype>
#include <algorithm>
#include <nlohmann/json.hpp>
#include "httplib.h"
#include <process.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <mutex>
#include <iphlpapi.h>
#include <intrin.h>
#include <iomanip>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "iphlpapi.lib")

using json = nlohmann::json;

struct AccountConfig {
    std::string name;
    std::string password;
    std::string totpSecret;
    int slot;
    std::string args;
};

struct GlobalConfig {
    int delay;
    bool POLProxy;
    std::vector<AccountConfig> accounts;
};

#define SHA1_BLOCK_SIZE 64
#define SHA1_DIGEST_LENGTH 20

std::string readIni(const std::string& path, const std::string& key) {
    std::ifstream file(path);
    if (!file) return "";
    std::string line;
    while (std::getline(file, line)) {
        auto pos = line.find('=');
        if (pos == std::string::npos) continue;
        std::string k = line.substr(0, pos), v = line.substr(pos + 1);
        if (k == key) return v;
    }
    return "";
}

std::string base32_decode(const std::string& input) {
    const char* alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    std::vector<uint8_t> output;
    int buffer = 0, bitsLeft = 0;
    for (char c : input) {
        if (c == '=' || c == ' ') break;
        const char* p = strchr(alphabet, toupper(c));
        if (!p) continue;
        buffer <<= 5;
        buffer |= (p - alphabet);
        bitsLeft += 5;
        if (bitsLeft >= 8) {
            output.push_back((buffer >> (bitsLeft - 8)) & 0xFF);
            bitsLeft -= 8;
        }
    }
    return std::string(output.begin(), output.end());
}

// SHA1 implementation public domain (Steve Reid, others)
void sha1(const uint8_t* data, size_t len, uint8_t* out);

void hmac_sha1(const uint8_t* key, size_t key_len, const uint8_t* data, size_t data_len, uint8_t output[20]) {
    const size_t blockSize = 64;
    const size_t hashSize = 20;
    uint8_t k_ipad[blockSize] = { 0 };
    uint8_t k_opad[blockSize] = { 0 };
    uint8_t tk[hashSize] = { 0 };

    if (key_len > blockSize) {
        sha1(key, key_len, tk);
        key = tk;
        key_len = hashSize;
    }

    uint8_t k0[blockSize] = { 0 };
    memcpy(k0, key, key_len);

    for (size_t i = 0; i < blockSize; ++i) {
        k_ipad[i] = k0[i] ^ 0x36;
        k_opad[i] = k0[i] ^ 0x5c;
    }

    std::vector<uint8_t> inner_data;
    inner_data.insert(inner_data.end(), k_ipad, k_ipad + blockSize);
    inner_data.insert(inner_data.end(), data, data + data_len);

    uint8_t inner_hash[hashSize];
    sha1(inner_data.data(), inner_data.size(), inner_hash);

    std::vector<uint8_t> outer_data;
    outer_data.insert(outer_data.end(), k_opad, k_opad + blockSize);
    outer_data.insert(outer_data.end(), inner_hash, inner_hash + hashSize);

    sha1(outer_data.data(), outer_data.size(), output);
}

std::string generate_totp(const std::string& secret_base32) {
    std::string key = base32_decode(secret_base32);
    uint64_t timestep = time(nullptr) / 30;
    uint8_t msg[8];
    for (int i = 7; i >= 0; --i) {
        msg[i] = timestep & 0xFF;
        timestep >>= 8;
    }

    uint8_t hash[20];
    hmac_sha1((uint8_t*)key.data(), key.size(), msg, 8, hash);

    int offset = hash[19] & 0x0F;
    int binary =
        ((hash[offset] & 0x7F) << 24) |
        ((hash[offset + 1] & 0xFF) << 16) |
        ((hash[offset + 2] & 0xFF) << 8) |
        (hash[offset + 3] & 0xFF);

    int code = binary % 1000000;
    char buf[7];
    snprintf(buf, sizeof(buf), "%06d", code);
    return std::string(buf);
}

// Base64 encoding for safe JSON storage
std::string base64Encode(const std::string& data) {
    const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    int val = 0, valb = -6;
    for (unsigned char c : data) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            result.push_back(chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) result.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
    while (result.size() % 4) result.push_back('=');
    return result;
}

std::string base64Decode(const std::string& data) {
    const int T[128] = {
        -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
        -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
        -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,62, -1,-1,-1,63,
        52,53,54,55, 56,57,58,59, 60,61,-1,-1, -1,-1,-1,-1,
        -1, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,
        15,16,17,18, 19,20,21,22, 23,24,25,-1, -1,-1,-1,-1,
        -1,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,
        41,42,43,44, 45,46,47,48, 49,50,51,-1, -1,-1,-1,-1
    };
    std::string result;
    int val = 0, valb = -8;
    for (unsigned char c : data) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            result.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return result;
}

// Machine fingerprinting for encryption key generation
std::string getCurrentUsername() {
    char username[256];
    DWORD size = sizeof(username);
    if (GetUserNameA(username, &size)) {
        return std::string(username);
    }
    return "unknown";
}

std::string getCPUInfo() {
    int cpuInfo[4];
    __cpuid(cpuInfo, 0);
    std::stringstream ss;
    ss << cpuInfo[1] << cpuInfo[3] << cpuInfo[2]; // EBX, EDX, ECX
    return ss.str();
}

std::string getFirstNetworkMAC() {
    IP_ADAPTER_INFO adapterInfo[16];
    DWORD bufLen = sizeof(adapterInfo);
    DWORD status = GetAdaptersInfo(adapterInfo, &bufLen);
    if (status == ERROR_SUCCESS) {
        std::stringstream ss;
        for (int i = 0; i < 6; i++) {
            ss << std::hex << (int)adapterInfo[0].Address[i];
        }
        return ss.str();
    }
    return "nomac";
}

std::string generateMachineKey() {
    std::string machineData = 
        getCurrentUsername() + 
        getCPUInfo() + 
        getFirstNetworkMAC();
    
    // Generate SHA1 hash of machine data
    uint8_t hash[20];
    sha1((uint8_t*)machineData.data(), machineData.size(), hash);
    
    // Convert to hex string for 128-bit key
    std::stringstream ss;
    for (int i = 0; i < 16; i++) {
        ss << std::hex << std::setfill('0') << std::setw(2) << (int)hash[i];
    }
    return ss.str();
}

bool isEncrypted(const std::string& data) {
    // Check if string looks like base64 (basic heuristic)
    if (data.empty()) return false;
    
    // Base64 strings are typically longer and contain specific characters
    if (data.length() < 8) return false;
    
    // Check for base64 characters
    for (char c : data) {
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || 
              (c >= '0' && c <= '9') || c == '+' || c == '/' || c == '=')) {
            return false;
        }
    }
    
    // Additional check: try to decode and see if it makes sense
    try {
        std::string decoded = base64Decode(data);
        return decoded.length() > 0;
    } catch (...) {
        return false;
    }
}

// Simple XOR encryption with machine-specific key
std::string encryptData(const std::string& data) {
    if (data.empty()) return "";
    
    std::string key = generateMachineKey();
    std::string encrypted;
    
    for (size_t i = 0; i < data.size(); i++) {
        encrypted += (char)(data[i] ^ key[i % key.size()]);
    }
    
    return base64Encode(encrypted);
}

std::string decryptData(const std::string& encryptedData) {
    if (encryptedData.empty()) return "";
    
    // Check if data is encrypted (base64 encoded)
    if (!isEncrypted(encryptedData)) {
        return encryptedData; // Return as-is if not encrypted
    }
    
    try {
        std::string decoded = base64Decode(encryptedData);
        std::string key = generateMachineKey();
        std::string decrypted;
        
        for (size_t i = 0; i < decoded.size(); i++) {
            decrypted += (char)(decoded[i] ^ key[i % key.size()]);
        }
        
        return decrypted;
    } catch (...) {
        return encryptedData; // Return original if decryption fails
    }
}

void simulateKey(WORD vk, bool shift = false) {
    INPUT inputs[4] = {};
    int count = 0;
    if (shift) {
        inputs[count].type = INPUT_KEYBOARD;
        inputs[count].ki.wVk = VK_SHIFT;
        count++;
    }
    inputs[count].type = INPUT_KEYBOARD;
    inputs[count].ki.wVk = vk;
    count++;
    inputs[count].type = INPUT_KEYBOARD;
    inputs[count].ki.wVk = vk;
    inputs[count].ki.dwFlags = KEYEVENTF_KEYUP;
    count++;
    if (shift) {
        inputs[count].type = INPUT_KEYBOARD;
        inputs[count].ki.wVk = VK_SHIFT;
        inputs[count].ki.dwFlags = KEYEVENTF_KEYUP;
        count++;
    }
    SendInput(count, inputs, sizeof(INPUT));
    Sleep(30);
}

void sendText(HWND hwnd, const std::string& text, int delay = 50) {
    for (char c : text) {
        SHORT vk = VkKeyScanA(c);
        if (vk == -1) continue;
        bool shift = (vk & 0x0100) != 0;
        WORD vkCode = vk & 0xFF;
        SetForegroundWindow(hwnd);
        simulateKey(vkCode, shift);
        Sleep(delay);
    }
}

bool isValidIP(const std::string& ip) {
    int parts = 0;
    std::istringstream ss(ip);
    std::string token;
    while (std::getline(ss, token, '.')) {
        if (++parts > 4) return false;
        int val = atoi(token.c_str());
        if (val < 0 || val > 255) return false;
    }
    return parts == 4;
}

void addHostsEntry(const std::string& ip) {
    std::ifstream in("C:\\Windows\\System32\\drivers\\etc\\hosts");
    std::string line;
    while (std::getline(in, line)) {
        if (line.find("wh000.pol.com") != std::string::npos && line.find("#ffxi-autologin") != std::string::npos)
            return; // Entry already exists
    }
    in.close();

    std::ofstream out("C:\\Windows\\System32\\drivers\\etc\\hosts", std::ios::app);
    out << "\n" << ip << " wh000.pol.com #ffxi-autologin\n";
}

void removeHostsEntry() {
    const char* path = "C:\\Windows\\System32\\drivers\\etc\\hosts";
    const char* tmpPath = "C:\\Windows\\System32\\drivers\\etc\\hosts.tmp";

    std::ifstream in(path);
    std::ofstream out(tmpPath);

    std::string line;
    while (std::getline(in, line)) {
        // Trim leading/trailing whitespace
        std::string trimmed = line;
        trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
        trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);

        // Skip blank or matching lines
        if (trimmed.empty() || trimmed.find("#ffxi-autologin") != std::string::npos)
            continue;

        out << line << "\n";
    }

    in.close();
    out.close();

    DeleteFileA(path);
    MoveFileA(tmpPath, path);
}

// Define a struct for passing data to EnumWindowsProc
struct WindowSearchData {
    const std::wstring* username;
    HWND* foundHwnd;
};

BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam) {
    wchar_t title[256];
    GetWindowTextW(hWnd, title, sizeof(title) / sizeof(wchar_t));
    if (wcsncmp(title, L"PlayOnline Viewer", 17) == 0) {
        WindowSearchData* data = reinterpret_cast<WindowSearchData*>(lParam);
        // Create a new title with username only
        std::wstring newTitle = L"PlayOnline Viewer - ";
        newTitle += *(data->username);
        SetWindowTextW(hWnd, newTitle.c_str());
        // Bring this window to the front and focus it
        SetForegroundWindow(hWnd);
        SetActiveWindow(hWnd);
        SetFocus(hWnd);
        BringWindowToTop(hWnd);
        *(data->foundHwnd) = hWnd;
        return FALSE;
    }
    return TRUE;
}

std::string readConfigFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void writeConfigFile(const std::string& path, const GlobalConfig& config) {
    json j;
    j["delay"] = config.delay;
    json accounts = json::array();
    for (const auto& account : config.accounts) {
        json acc;
        // Encrypt sensitive fields
        acc["name"] = encryptData(account.name);
        acc["password"] = encryptData(account.password);
        acc["totpSecret"] = encryptData(account.totpSecret);
        acc["slot"] = account.slot;
        acc["args"] = account.args;
        accounts.push_back(acc);
    }
    j["accounts"] = accounts;
    std::ofstream file(path);
    file << j.dump(4);
}

GlobalConfig loadConfig(const std::string& path) {
    GlobalConfig config;
    std::string content = readConfigFile(path);
    if (content.empty()) {
        return config;
    }
    try {
        json j = json::parse(content);
        config.delay = j.value("delay", 1000);
        config.POLProxy = true;
        if (j.contains("accounts")) {
            for (const auto& acc : j["accounts"]) {
                AccountConfig account;
                // Decrypt sensitive fields
                account.name = decryptData(acc.value("name", ""));
                account.password = decryptData(acc["password"]);
                account.totpSecret = decryptData(acc["totpSecret"]);
                account.slot = acc["slot"];
                account.args = acc.value("args", "");
                config.accounts.push_back(account);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing config file: " << e.what() << std::endl;
    }
    return config;
}

void setupConfig(GlobalConfig& config) {
    std::cout << "\nCreated by: jaku  |  https://twitter.com/jaku\n";
    std::cout << "Setting up FFXI AutoLogin configuration\n";
    std::string input;
    std::cout << "Delay before input starts (in seconds, default 1. Some PCs may need this to be higher.): ";
    std::getline(std::cin, input);
    if (!input.empty() && std::all_of(input.begin(), input.end(), ::isdigit)) {
        int val = std::stoi(input);
        if (val >= 1 && val <= 20) config.delay = val * 1000;
    }
    int numAccounts = 0;
    while (true) {
        std::cout << "How many characters do you want to set up? ";
        std::getline(std::cin, input);
        if (!input.empty() && std::all_of(input.begin(), input.end(), ::isdigit)) {
            numAccounts = std::stoi(input);
            if (numAccounts > 0) break;
        }
        std::cout << "Please enter a valid number greater than 0.\n";
    }
    for (int i = 0; i < numAccounts; i++) {
        std::cout << "\nSetting up character " << (i + 1) << ":\n";
        AccountConfig account;
        // Name (no spaces)
        while (true) {
            std::cout << "Character name (no spaces, unique): ";
            std::getline(std::cin, account.name);
            if (account.name.find(' ') != std::string::npos || account.name.empty()) {
                std::cout << "Name cannot contain spaces or be empty. Try again.\n";
                continue;
            }
            bool duplicate = false;
            for (const auto& acc : config.accounts) {
                if (_stricmp(acc.name.c_str(), account.name.c_str()) == 0) {
                    duplicate = true;
                    break;
                }
            }
            if (duplicate) {
                std::cout << "Name must be unique. Try again.\n";
                continue;
            }
            break;
        }
        std::cout << "Password: ";
        std::getline(std::cin, account.password);
        std::cout << "TOTP Secret (leave empty if not using): ";
        std::getline(std::cin, account.totpSecret);
        // Slot (1-4)
        while (true) {
            std::cout << "Slot number (1-4): ";
            std::getline(std::cin, input);
            if (!input.empty() && std::all_of(input.begin(), input.end(), ::isdigit)) {
                int slot = std::stoi(input);
                if (slot >= 1 && slot <= 4) {
                    account.slot = slot;
                    break;
                }
            }
            std::cout << "Slot must be 1, 2, 3, or 4. Try again.\n";
        }
        std::cout << "Windower arguments (e.g. -p=\"ProfileName\" leave empty for none) ";
        std::getline(std::cin, account.args);
        config.accounts.push_back(account);
    }
}

int getLoginWValue(const std::string& polPath) {
    std::string loginWPath = polPath + "\\usr\\all\\login_w.bin";
    std::ifstream file(loginWPath, std::ios::binary);
    if (!file) {
        std::cerr << "Could not open login_w.bin at: " << loginWPath << "\n";
        return -1;
    }

    // Seek to offset 0x64
    file.seekg(0x64);
    if (file.fail()) {
        std::cerr << "Failed to seek to offset 0x64 in login_w.bin\n";
        return -1;
    }

    // Read the byte at that offset
    unsigned char value;
    file.read(reinterpret_cast<char*>(&value), 1);
    if (file.fail()) {
        std::cerr << "Failed to read value from login_w.bin\n";
        return -1;
    }

    return value;
}

std::string getPOLPath(DWORD processId) {
    //std::cout << "Looking for POL.exe in process " << processId << "\n";
    
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return "";
    }

    MODULEENTRY32W moduleEntry;
    moduleEntry.dwSize = sizeof(moduleEntry);
    if (!Module32FirstW(snapshot, &moduleEntry)) {
        std::cerr << "Module32First failed with error: " << GetLastError() << "\n";
        CloseHandle(snapshot);
        return "";
    }

    std::string polPath;
    do {
        std::wstring moduleName = moduleEntry.szModule;
        
        if (_wcsicmp(moduleName.c_str(), L"pol.exe") == 0) {
            // Convert wide string to narrow string
            int size = WideCharToMultiByte(CP_UTF8, 0, moduleEntry.szExePath, -1, NULL, 0, NULL, NULL);
            std::string path(size, 0);
            WideCharToMultiByte(CP_UTF8, 0, moduleEntry.szExePath, -1, &path[0], size, NULL, NULL);
            polPath = path;
            //std::cout << "Found POL.exe at: " << polPath << "\n";
            break;
        }
    } while (Module32NextW(snapshot, &moduleEntry));

    if (polPath.empty()) {
        std::cerr << "POL.exe not found.\n";
    }

    CloseHandle(snapshot);
    return polPath;
}

// Helper to defocus any existing PlayOnline Viewer window
void defocusExistingPOL() {
    HWND fg = GetForegroundWindow();
    wchar_t title[256];
    if (fg && GetWindowTextW(fg, title, 256)) {
        if (wcsncmp(title, L"PlayOnline Viewer", 17) == 0 ||
            wcsncmp(title, L"Final Fantasy XI", 16) == 0) {
            // Set focus to desktop
            HWND desktop = GetDesktopWindow();
            SetForegroundWindow(desktop);
            Sleep(100);
        }
    }
}

void launchAccount(const AccountConfig& account, const GlobalConfig& config) {
    if (config.POLProxy) {
        addHostsEntry("127.0.0.1");
    }


    std::cout << "Launching character: " << account.name << std::endl;

    // Defocus any existing POL window before launching new one
    defocusExistingPOL();

    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    PathRemoveFileSpecA(exePath);
    std::string baseDir = exePath;

    std::string exe = baseDir + "\\Windower.exe";
    bool isWindower = true;
    if (GetFileAttributesA(exe.c_str()) == INVALID_FILE_ATTRIBUTES) {
        exe = baseDir + "\\pol.exe";
        isWindower = false;
    }

    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    std::string cmdline = exe;
    if (isWindower && !account.args.empty()) {
        cmdline += " " + account.args;
    }

    if (!CreateProcessA(NULL, (LPSTR)cmdline.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        std::cerr << "Failed to launch, try running as admin.\n";
        return;
    }

    //std::cout << "Launched process with ID: " << pi.dwProcessId << "\n";

    // Convert username to wide string for window title
    std::wstring wUsername(account.name.begin(), account.name.end());
    HWND hwnd = nullptr;
    WindowSearchData searchData = { &wUsername, &hwnd };
    for (int i = 0; i < 60 && !hwnd; ++i) {
        EnumWindows(EnumWindowsProc, (LPARAM)&searchData);
        if (!hwnd) Sleep(500);
    }

    if (!hwnd) {
        std::cerr << "Could not find POL window\n";
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return;
    }

    // Get the actual POL.exe path from the running process
    std::string polPath = getPOLPath(pi.dwProcessId);
    if (polPath.empty()) {
        // Try getting the path from the window
        char windowPath[MAX_PATH];
        DWORD processId;
        GetWindowThreadProcessId(hwnd, &processId);
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
        if (hProcess) {
            if (GetModuleFileNameExA(hProcess, NULL, windowPath, MAX_PATH)) {
                polPath = windowPath;
                //std::cout << "Found POL.exe through window handle: " << polPath << "\n";
            }
            CloseHandle(hProcess);
        }
    }

    if (polPath.empty()) {
        std::cerr << "Could not find POL.exe path\n";
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return;
    }

    // Get the directory containing POL.exe
    size_t lastSlash = polPath.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        polPath = polPath.substr(0, lastSlash);
    }

    // Read login_w.bin value
    int loginWValue = getLoginWValue(polPath);
    if (loginWValue == -1) {
        std::cerr << "Failed to read login_w.bin, using default slot selection\n";
    } else {
        //std::cout << "Read slot value from login_w.bin: " << loginWValue << "\n";
    }

    // Wait for the window to have a title bar (WS_CAPTION)
    int waitTitleBar = 0;
    while (!(GetWindowLong(hwnd, GWL_STYLE) & WS_CAPTION) && waitTitleBar < 100) { // up to 10s
        Sleep(100);
        waitTitleBar++;
    }

    // if (waitTitleBar >= 100) {
    //     //std::wcout << L"[WARN] Window did not get a title bar in time! Proceeding anyway." << std::endl;
    // } else {
    //     //std::wcout << L"[INFO] Window has title bar, proceeding with focus and input." << std::endl;
    // }

    // Logging before BlockInput(TRUE)
    DWORD winPid = 0;
    GetWindowThreadProcessId(hwnd, &winPid);
    wchar_t winTitle[256] = {0};
    GetWindowTextW(hwnd, winTitle, 256);
    //std::wcout << L"[INFO] About to focus window HWND: 0x" << std::hex << (uintptr_t)hwnd
    //           << L" | Title: '" << winTitle << L"' | PID: " << std::dec << winPid << std::endl;

    BlockInput(TRUE);

    SetForegroundWindow(hwnd);
    SetActiveWindow(hwnd);
    SetFocus(hwnd);
    BringWindowToTop(hwnd);
    // Send VK_MENU (Alt) to help force focus
    keybd_event(VK_MENU, 0, 0, 0);
    keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);

    //Sleep(config.delay);
    // Extra focus before input
    SetForegroundWindow(hwnd);
    SetActiveWindow(hwnd);
    SetFocus(hwnd);
    BringWindowToTop(hwnd);

    // Adjust slot selection based on login_w.bin value
    if (loginWValue != -1) {
        int targetSlot = account.slot;
        if (targetSlot < loginWValue) {
            // If we want a lower slot than what's in the file, we need to press UP
            int upPresses = loginWValue - targetSlot;
            //std::cout << "Need to press UP " << upPresses << " times to reach slot " << targetSlot << "\n";
            for (int i = 0; i < upPresses; ++i) {
                simulateKey(VK_UP);
                Sleep(200);
            }
        } else if (targetSlot > loginWValue) {
            // If we want a higher slot than what's in the file, we need to press DOWN
            int downPresses = targetSlot - loginWValue;
            //std::cout << "Need to press DOWN " << downPresses << " times to reach slot " << targetSlot << "\n";
            for (int i = 0; i < downPresses; ++i) {
                simulateKey(VK_DOWN);
                Sleep(200);
            }
        }
    } else {
        // Fallback to original slot selection if we couldn't read login_w.bin
        if (account.slot > 1) {
            for (int i = 1; i < account.slot; ++i) {
                simulateKey(VK_DOWN);
                Sleep(200);
            }
        }
    }

    SetForegroundWindow(hwnd);
    SetActiveWindow(hwnd);
    SetFocus(hwnd);
    BringWindowToTop(hwnd);

    Sleep(200);
    simulateKey(VK_RETURN);
    Sleep(200);
    simulateKey(VK_RETURN);
    Sleep(300);
    simulateKey(VK_RETURN);
    Sleep(500);
    simulateKey(VK_RETURN);
    Sleep(500);

    sendText(hwnd, account.password, 5);
    Sleep(100);

    simulateKey(VK_RETURN);
    Sleep(500);
    simulateKey(VK_DOWN);
    Sleep(300);

    if (!account.totpSecret.empty()) {
        simulateKey(VK_RETURN);
        std::string totp = generate_totp(account.totpSecret);
        sendText(hwnd, totp, 5);
        simulateKey(VK_ESCAPE);
        Sleep(100);
        simulateKey(VK_DOWN);
        Sleep(100);
    }

    simulateKey(VK_RETURN);
    Sleep(50);

    simulateKey(VK_RETURN);
    Sleep(500);

    BlockInput(FALSE);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

// Add a global vector to store accounts to launch
std::vector<AccountConfig> accountsToLaunch;
std::atomic<int> currentAccountIndex(0);
std::atomic<bool> shouldExit(false);

void startProxyServer() {
    
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Server failed to start\n";
        return;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Port creation failed\n";
        WSACleanup();
        return;
    }
    //std::cout << "Port opened successfully\n";

    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0) {
        std::cerr << "setsockopt failed\n";
        closesocket(serverSocket);
        WSACleanup();
        return;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);
    serverAddr.sin_port = htons(51304);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed with error: " << WSAGetLastError() << "\n";
        closesocket(serverSocket);
        WSACleanup();
        return;
    }
    //std::cout << "Port opened successfully to 127.0.0.1:51304\n";

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed with error: " << WSAGetLastError() << "\n";
        closesocket(serverSocket);
        WSACleanup();
        return;
    }

    while (!shouldExit) {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed with error: " << WSAGetLastError() << "\n";
            continue;
        }
        char buffer[4096];
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            if (strstr(buffer, "GET /pml/main/index.pml") != nullptr) {
                std::string response = "HTTP/1.1 200 OK\r\n"
                                     "Content-Type: text/x-playonline-pml;charset=UTF-8\r\n"
                                     "Content-Length: 123\r\n"
                                     "Connection: close\r\n"
                                     "\r\n"
                                     "<pml><head><meta http-equiv=\"Content-Type\" content=\"text/x-playonline-pml;charset=UTF-8\"><title>Fast</title></head><body><timer name=\"fast\" href=\"gameto:1\" enable=\"1\" delay=\"0\"></body></pml>";
                send(clientSocket, response.c_str(), response.length(), 0);
                shouldExit = true;
            } else {
                std::string response = "HTTP/1.1 404 Not Found\r\n"
                                     "Content-Type: text/plain\r\n"
                                     "Content-Length: 13\r\n"
                                     "Connection: close\r\n"
                                     "\r\n"
                                     "Not Found";
                send(clientSocket, response.c_str(), response.length(), 0);
                std::cout << "Sent 404 response\n";
            }
        }
        closesocket(clientSocket);
    }
    closesocket(serverSocket);
    WSACleanup();
}

int main(int argc, char* argv[]) {
    std::cout << "Created by: jaku  |  https://twitter.com/jaku\n";
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    PathRemoveFileSpecA(exePath);
    std::string baseDir = exePath;
    std::string configPath = baseDir + "\\config.json";
    GlobalConfig config = loadConfig(configPath);
    bool setupMode = false;
    std::string characterName;
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--setup") {
            setupMode = true;
        } else if (arg == "--character" && i + 1 < argc) {
            characterName = argv[++i];
        }
    }
    if (setupMode || config.accounts.empty()) {
        setupConfig(config);
        writeConfigFile(configPath, config);
        std::cout << "Setup complete. Exiting.\n";
        return 0;
    }
    if (config.accounts.empty()) {
        std::cout << "No accounts configured. Please run with --setup to configure accounts.\n";
        return 1;
    }
    // If more than one account and no character specified, prompt user
    if (characterName.empty() && config.accounts.size() > 1) {
        std::cout << "\nSelect a character to log in with:\n";
        for (size_t i = 0; i < config.accounts.size(); ++i) {
            std::cout << "  [" << (i + 1) << "] " << config.accounts[i].name << " (slot " << config.accounts[i].slot << ")\n";
        }
        std::string input;
        int choice = 0;
        while (true) {
            std::cout << "Enter number (1-" << config.accounts.size() << "): ";
            std::getline(std::cin, input);
            if (!input.empty() && std::all_of(input.begin(), input.end(), ::isdigit)) {
                choice = std::stoi(input);
                if (choice >= 1 && (size_t)choice <= config.accounts.size()) {
                    characterName = config.accounts[choice - 1].name;
                    break;
                }
            }
            std::cout << "Invalid choice. Try again.\n";
        }
    }
    // Always start proxy server
    std::thread proxyThread(startProxyServer);
    // Find the account to launch
    AccountConfig* toLaunch = nullptr;
    if (characterName.empty()) {
        // Default: launch the first slot
        for (auto& acc : config.accounts) {
            if (acc.slot == 1) { toLaunch = &acc; break; }
        }
    } else {
        for (auto& acc : config.accounts) {
            if (_stricmp(acc.name.c_str(), characterName.c_str()) == 0) { toLaunch = &acc; break; }
        }
    }
    if (!toLaunch) {
        std::cout << "No account found for requested character name.\n";
        return 1;
    }
    launchAccount(*toLaunch, config);
    // Wait for a request, then exit
    while (!shouldExit) { Sleep(100); }
    proxyThread.join();
    return 0;
}
