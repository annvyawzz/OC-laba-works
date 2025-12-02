#pragma once
#include <windows.h>
#include <string>

class Browser 
{
public:
    Browser();
    ~Browser();

    bool Initialize();
    void Run();
    void Close();

private:
    HANDLE m_downloadSlots;
    HANDLE m_logMutex;
    HANDLE m_browserEvent;
    HANDLE* m_downloaderProcesses;
    int m_maxDownloads;
    int m_totalFiles;

    bool CreateSyncObjects();
    bool StartDownloaders();
    void Cleanup();
};