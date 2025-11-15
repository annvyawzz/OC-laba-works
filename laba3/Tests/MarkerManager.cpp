#include "pch.h"
#include "MarkerManager.h"
#include <iostream>

MarkerManager::MarkerManager()
    : arraySize(0), markerCount(0), sharedArray(nullptr),
    suspendEvents(nullptr), terminateEvents(nullptr), continueEvents(nullptr),
    startEvent(nullptr), markerThreads(nullptr), threadData(nullptr) {
    // Инициализируем критические секции
    InitializeCriticalSection(&cs);
    InitializeCriticalSection(&outputCs);
}

MarkerManager::~MarkerManager() {
    Cleanup();
}

bool MarkerManager::Initialize(int arrSize, int mCount) {
    if (arrSize <= 0 || mCount <= 0) {
        return false;
    }

    arraySize = arrSize;
    markerCount = mCount;

    sharedArray = new int[arraySize]();
    activeMarkers.resize(markerCount, true);

    CreateSyncObjects();
    CreateThreads();

    return true;
}

void MarkerManager::CreateSyncObjects() {
    suspendEvents = new HANDLE[markerCount]();
    terminateEvents = new HANDLE[markerCount]();
    continueEvents = new HANDLE[markerCount]();
    startEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    for (int i = 0; i < markerCount; ++i) {
        suspendEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
        terminateEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
        continueEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
    }
}

void MarkerManager::CreateThreads() {
    markerThreads = new HANDLE[markerCount]();
    threadData = new MarkerThreadData[markerCount]();

    for (int i = 0; i < markerCount; ++i) {
        threadData[i] = {
            i + 1,
            arraySize,
            sharedArray,
            &cs,
            &outputCs,
            startEvent,
            suspendEvents[i],
            terminateEvents[i],
            continueEvents[i]
        };

        markerThreads[i] = CreateThread(
            NULL,
            0,
            MarkerThread,
            &threadData[i],
            0,
            NULL
        );

        if (markerThreads[i] == NULL) {
            std::cerr << "Ошибка создания потока " << (i + 1) << std::endl;
        }
    }
}

void MarkerManager::Run() {
    std::cout << "Создано " << markerCount << " потоков marker" << std::endl;
    SetEvent(startEvent);
    std::cout << "Все потоки запущены одновременно" << std::endl;

    MainLoop();
}

void MarkerManager::TerminateThread(int threadIndex) {
    if (threadIndex >= 0 && threadIndex < markerCount && activeMarkers[threadIndex]) {
        SetEvent(terminateEvents[threadIndex]);
        WaitForSingleObject(markerThreads[threadIndex], INFINITE);
        CloseHandle(markerThreads[threadIndex]);
        activeMarkers[threadIndex] = false;
    }
}

void MarkerManager::MainLoop() {
    int activeCount = markerCount;

    while (activeCount > 0) {
        std::vector<HANDLE> activeSuspendEvents;
        std::vector<int> activeIndices;

        for (int i = 0; i < markerCount; ++i) {
            if (activeMarkers[i]) {
                activeSuspendEvents.push_back(suspendEvents[i]);
                activeIndices.push_back(i);
            }
        }

        if (activeSuspendEvents.empty()) break;

        DWORD waitResult = WaitForMultipleObjects(
            (DWORD)activeSuspendEvents.size(),
            activeSuspendEvents.data(),
            FALSE,
            INFINITE
        );

        if (waitResult == WAIT_FAILED) {
            std::cout << "Ошибка при ожидании событий" << std::endl;
            break;
        }

        int eventIndex = waitResult - WAIT_OBJECT_0;
        if (eventIndex < 0 || eventIndex >= (int)activeIndices.size()) {
            continue;
        }

        int suspendedThreadIndex = activeIndices[eventIndex];
        Sleep(50);

        EnterCriticalSection(&cs);
        std::cout << "Текущее состояние массива: ";
        for (int i = 0; i < arraySize; ++i) {
            std::cout << sharedArray[i] << " ";
        }
        std::cout << std::endl;
        LeaveCriticalSection(&cs);

        int threadToTerminate;
        std::cout << "Введите номер потока marker для завершения (1-"
            << markerCount << "): ";
        std::cin >> threadToTerminate;

        if (threadToTerminate < 1 || threadToTerminate > markerCount ||
            !activeMarkers[threadToTerminate - 1]) {
            std::cout << "Неверный номер потока! Попробуйте снова." << std::endl;

            for (int i = 0; i < markerCount; ++i) {
                if (activeMarkers[i]) {
                    SetEvent(continueEvents[i]);
                }
            }
            continue;
        }

        int threadIndex = threadToTerminate - 1;

        std::cout << "Завершение потока " << threadToTerminate << "..." << std::endl;
        TerminateThread(threadIndex);
        activeCount--;

        EnterCriticalSection(&cs);
        std::cout << "Состояние массива после завершения потока " << threadToTerminate << ": ";
        for (int i = 0; i < arraySize; ++i) {
            std::cout << sharedArray[i] << " ";
        }
        std::cout << std::endl;
        LeaveCriticalSection(&cs);

        if (activeCount > 0) {
            std::cout << "Возобновление работы оставшихся " << activeCount << " потоков..." << std::endl;
            for (int i = 0; i < markerCount; ++i) {
                if (activeMarkers[i]) {
                    SetEvent(continueEvents[i]);
                }
            }
        }

        std::cout << "Активных потоков осталось: " << activeCount << std::endl;
    }

    std::cout << "\nВсе потоки marker завершены!" << std::endl;
}

void MarkerManager::Cleanup() {
    // Завершаем все активные потоки
    for (int i = 0; i < markerCount; ++i) {
        if (activeMarkers[i] && terminateEvents && terminateEvents[i]) {
            SetEvent(terminateEvents[i]);
        }
        if (markerThreads && markerThreads[i]) {
            WaitForSingleObject(markerThreads[i], INFINITE);
            CloseHandle(markerThreads[i]);
        }
    }

    if (sharedArray) {
        delete[] sharedArray;
        sharedArray = nullptr;
    }

    DeleteCriticalSection(&cs);
    DeleteCriticalSection(&outputCs);

    if (startEvent) {
        CloseHandle(startEvent);
        startEvent = nullptr;
    }

    if (suspendEvents) {
        for (int i = 0; i < markerCount; ++i) {
            if (suspendEvents[i]) CloseHandle(suspendEvents[i]);
        }
        delete[] suspendEvents;
        suspendEvents = nullptr;
    }

    if (terminateEvents) {
        for (int i = 0; i < markerCount; ++i) {
            if (terminateEvents[i]) CloseHandle(terminateEvents[i]);
        }
        delete[] terminateEvents;
        terminateEvents = nullptr;
    }

    if (continueEvents) {
        for (int i = 0; i < markerCount; ++i) {
            if (continueEvents[i]) CloseHandle(continueEvents[i]);
        }
        delete[] continueEvents;
        continueEvents = nullptr;
    }

    if (markerThreads) {
        delete[] markerThreads;
        markerThreads = nullptr;
    }

    if (threadData) {
        delete[] threadData;
        threadData = nullptr;
    }

    arraySize = 0;
    markerCount = 0;
    activeMarkers.clear();
}