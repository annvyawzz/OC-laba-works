#include "Browser.h"
#include <iostream>
#include <string>
#include <vector>

Browser::Browser()
    : m_downloadSlots(NULL), m_logMutex(NULL), m_browserEvent(NULL),
    m_downloaderProcesses(nullptr), m_maxDownloads(0), m_totalFiles(0) {
}

Browser::~Browser() {
    Cleanup();
}

bool Browser::Initialize() {
    std::cout << "Enter maximum number of simultaneous downloads (N): ";
    std::cin >> m_maxDownloads;

    std::cout << "Enter total number of files to download (M, M > N): ";
    std::cin >> m_totalFiles;

    if (m_totalFiles <= m_maxDownloads) {
        std::cout << "Error: M must be greater than N!" << std::endl;
        return false;
    }

    return CreateSyncObjects();
}

bool Browser::CreateSyncObjects() {
    // Create semaphore for download slots
    m_downloadSlots = CreateSemaphoreA(
        NULL,
        m_maxDownloads,
        m_maxDownloads,
        "DownloadSlots"
    );

    if (m_downloadSlots == NULL) {
        std::cout << "Failed to create semaphore: " << GetLastError() << std::endl;
        return false;
    }

    // Create mutex for log access
    m_logMutex = CreateMutexA(
        NULL,
        FALSE,
        "LogAccessMutex"
    );

    if (m_logMutex == NULL) {
        std::cout << "Failed to create mutex: " << GetLastError() << std::endl;
        return false;
    }

    // Create manual-reset event for browser closing
    m_browserEvent = CreateEventA(
        NULL,
        TRUE,
        FALSE,
        "BrowserClosingEvent"
    );

    if (m_browserEvent == NULL) {
        std::cout << "Failed to create event: " << GetLastError() << std::endl;
        return false;
    }

    return true;
}

bool Browser::StartDownloaders() {
    m_downloaderProcesses = new HANDLE[m_totalFiles];

    // Проверяем существование Downloader.exe
    FILE* file = fopen("Downloader.exe", "r");
    if (!file) {
        std::cout << "Error: Downloader.exe not found in current directory!" << std::endl;
        std::cout << "Current directory: ";
        system("cd"); // Показывает текущую директорию
        return false;
    }
    fclose(file);

    for (int i = 0; i < m_totalFiles; i++) {
        STARTUPINFOA si = { sizeof(si) };
        PROCESS_INFORMATION pi;

        std::string command = "Downloader.exe";

        if (CreateProcessA(
            NULL,
            const_cast<LPSTR>(command.c_str()),
            NULL,
            NULL,
            FALSE,
            0,
            NULL,
            NULL,
            &si,
            &pi
        )) {
            m_downloaderProcesses[i] = pi.hProcess;
            CloseHandle(pi.hThread);
            std::cout << "Started downloader process " << i + 1 << "/" << m_totalFiles << std::endl;
        }
        else {
            DWORD error = GetLastError();
            std::cout << "Failed to start downloader " << i << ": error " << error << std::endl;

            // Закрываем уже созданные процессы перед возвратом false
            for (int j = 0; j < i; j++) {
                if (m_downloaderProcesses[j]) {
                    TerminateProcess(m_downloaderProcesses[j], 0);
                    CloseHandle(m_downloaderProcesses[j]);
                }
            }
            delete[] m_downloaderProcesses;
            m_downloaderProcesses = nullptr;
            return false;
        }
    }

    return true;
}

void Browser::Run() {
    if (!StartDownloaders()) {
        return;
    }

    std::cout << "Browser is running. Press Enter to close..." << std::endl;
    std::cin.ignore();
    std::cin.get();

    Close();
}

void Browser::Close() {
    std::cout << "Browser is closing. Sending termination signal to all downloads..." << std::endl;

    SetEvent(m_browserEvent);

    WaitForMultipleObjects(m_totalFiles, m_downloaderProcesses, TRUE, INFINITE);

    std::cout << "All downloads terminated. Browser closing." << std::endl;
}

void Browser::Cleanup() {
    if (m_downloadSlots) {
        CloseHandle(m_downloadSlots);
        m_downloadSlots = NULL;
    }

    if (m_logMutex) {
        CloseHandle(m_logMutex);
        m_logMutex = NULL;
    }

    if (m_browserEvent) {
        CloseHandle(m_browserEvent);
        m_browserEvent = NULL;
    }

    if (m_downloaderProcesses) {
        for (int i = 0; i < m_totalFiles; i++) {
            if (m_downloaderProcesses[i]) {
                CloseHandle(m_downloaderProcesses[i]);
            }
        }
        delete[] m_downloaderProcesses;
        m_downloaderProcesses = nullptr;
    }
}