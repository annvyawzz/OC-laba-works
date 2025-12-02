#pragma once
#include <windows.h>
#include <string>

class Downloader
{
public:
    Downloader();
    ~Downloader();

    bool Initialize();
    void Run();

private:
    HANDLE m_downloadSlots;
    HANDLE m_logMutex;
    HANDLE m_browserEvent;

    bool OpenSyncObjects();
    void ProcessFile();
    void LogMessage(const std::string& message);

    void ReverseBytes();
    void CountWords();
    void SortChars();
    void CalculateStdDev();
    void CountBrackets();
    void FindLongestString();
    void CheckBalancedBrackets();
    void MatrixProduct();
    void CountPrimes();
};