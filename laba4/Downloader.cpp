#include "Downloader.h"
#include <iostream>
#include <string>
#include <random>
#include <algorithm>
#include <chrono>
#include <thread>
#include <vector>
#include <cmath>
#include <sstream>

Downloader::Downloader()
    : m_downloadSlots(NULL), m_logMutex(NULL), m_browserEvent(NULL) {
}

Downloader::~Downloader() {
    if (m_downloadSlots) CloseHandle(m_downloadSlots);
    if (m_logMutex) CloseHandle(m_logMutex);
    if (m_browserEvent) CloseHandle(m_browserEvent);
}

bool Downloader::Initialize() {
    return OpenSyncObjects();
}

bool Downloader::OpenSyncObjects() {
    m_downloadSlots = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, FALSE, "DownloadSlots");
    if (!m_downloadSlots) {
        return false;
    }

    m_logMutex = OpenMutexA(MUTEX_ALL_ACCESS, FALSE, "LogAccessMutex");
    if (!m_logMutex) {
        return false;
    }

    m_browserEvent = OpenEventA(EVENT_ALL_ACCESS, FALSE, "BrowserClosingEvent");
    if (!m_browserEvent) {
        return false;
    }

    return true;
}

void Downloader::LogMessage(const std::string& message) {
    WaitForSingleObject(m_logMutex, INFINITE);
    std::cout << "[PID: " << GetCurrentProcessId() << "] " << message << std::endl;
    ReleaseMutex(m_logMutex);
}

void Downloader::Run() {
    HANDLE waitHandles[2] = { m_downloadSlots, m_browserEvent };

    DWORD result = WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);

    if (result == WAIT_OBJECT_0) {
        LogMessage("Connection established. Starting download of 'file_" +
            std::to_string(GetCurrentProcessId()) + ".dat'...");

        std::this_thread::sleep_for(std::chrono::seconds(1 + (GetCurrentProcessId() % 3)));

        ProcessFile();

        LogMessage("File 'file_" + std::to_string(GetCurrentProcessId()) +
            ".dat' processed successfully.");

        ReleaseSemaphore(m_downloadSlots, 1, NULL);
    }
    else {
        LogMessage("Download interrupted - browser is closing.");
    }
}

void Downloader::ProcessFile() {
    int taskId = GetCurrentProcessId() % 9;

    switch (taskId) {
    case 0: ReverseBytes(); break;
    case 1: CountWords(); break;
    case 2: SortChars(); break;
    case 3: CalculateStdDev(); break;
    case 4: CountBrackets(); break;
    case 5: FindLongestString(); break;
    case 6: CheckBalancedBrackets(); break;
    case 7: MatrixProduct(); break;
    case 8: CountPrimes(); break;
    }
}

void Downloader::ReverseBytes() {
    const int SIZE = 2048;
    unsigned char data[SIZE];

    for (int i = 0; i < SIZE; i++) {
        data[i] = rand() % 256;
    }

    for (int i = 0; i < SIZE / 2; i++) {
        std::swap(data[i], data[SIZE - 1 - i]);
    }
}

void Downloader::CountWords() {
    std::string text = "This is a sample text with several words to count in this string";
    std::istringstream iss(text);
    int wordCount = 0;
    std::string word;

    while (iss >> word) {
        wordCount++;
    }
}

void Downloader::SortChars() {
    const int SIZE = 500;
    char chars[SIZE];

    for (int i = 0; i < SIZE; i++) {
        chars[i] = 'A' + (rand() % 26);
    }

    std::sort(chars, chars + SIZE);
}

void Downloader::CalculateStdDev() {
    const int SIZE = 200;
    double numbers[SIZE];
    double sum = 0, mean, stdDev = 0;

    for (int i = 0; i < SIZE; i++) {
        numbers[i] = rand() % 100;
        sum += numbers[i];
    }

    mean = sum / SIZE;

    for (int i = 0; i < SIZE; i++) {
        stdDev += pow(numbers[i] - mean, 2);
    }

    stdDev = sqrt(stdDev / SIZE);
}

void Downloader::CountBrackets() {
    std::string text = "This (is) a [sample] {text} with (various [types] of {brackets})";
    int openCount = 0, closeCount = 0;

    for (char c : text) {
        if (c == '(' || c == '[' || c == '{') openCount++;
        if (c == ')' || c == ']' || c == '}') closeCount++;
    }
}

void Downloader::FindLongestString() {
    std::vector<std::string> strings = {
        "short", "medium length", "this is the longest string", "another"
    };

    std::string longest = strings[0];
    for (const auto& str : strings) {
        if (str.length() > longest.length()) {
            longest = str;
        }
    }
}

void Downloader::CheckBalancedBrackets() {
    std::string text = "({[]})";
    std::vector<char> stack;
    bool balanced = true;

    for (char c : text) {
        if (c == '(' || c == '[' || c == '{') {
            stack.push_back(c);
        }
        else if (c == ')' || c == ']' || c == '}') {
            if (stack.empty()) {
                balanced = false;
                break;
            }

            char top = stack.back();
            stack.pop_back();

            if ((c == ')' && top != '(') ||
                (c == ']' && top != '[') ||
                (c == '}' && top != '{')) {
                balanced = false;
                break;
            }
        }
    }

    balanced = balanced && stack.empty();
}

void Downloader::MatrixProduct() {
    const int SIZE = 10;
    int matrix[SIZE][SIZE];
    long long product = 1;

    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            matrix[i][j] = 1 + (rand() % 5);
            product *= matrix[i][j];
        }
    }
}

void Downloader::CountPrimes() {
    const int LIMIT = 10000;
    int primeCount = 0;

    for (int i = 2; i <= LIMIT; i++) {
        bool isPrime = true;
        for (int j = 2; j * j <= i; j++) {
            if (i % j == 0) {
                isPrime = false;
                break;
            }
        }
        if (isPrime) primeCount++;
    }
}