#pragma once
#include "pch.h"
#include <windows.h>
#include <vector>
#include "MarkerThread.h"

class MarkerManager {
public:
    MarkerManager();
    ~MarkerManager();

    bool Initialize(int arraySize, int markerCount);
    void Run();
    void Cleanup();

    // Методы для тестирования
    int* GetSharedArray() const { return sharedArray; }
    int GetArraySize() const { return arraySize; }
    std::vector<bool> GetActiveMarkers() const { return activeMarkers; }
    void TerminateThread(int threadIndex);

    void ManualRun(); // Запуск без интерактивного ввода для тестов

private:
    int arraySize = 0;
    int markerCount = 0;
    int* sharedArray = nullptr;
    std::vector<bool> activeMarkers;

    CRITICAL_SECTION cs;
    CRITICAL_SECTION outputCs;

    HANDLE* suspendEvents = nullptr;
    HANDLE* terminateEvents = nullptr;
    HANDLE* continueEvents = nullptr;
    HANDLE startEvent = nullptr;
    HANDLE* markerThreads = nullptr;

    MarkerThreadData* threadData = nullptr;

    void CreateSyncObjects();
    void CreateThreads();
    void MainLoop();
};
