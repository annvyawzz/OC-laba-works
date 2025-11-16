#include "pch.h"
#include "MarkerThread.h"
#include <iostream>

DWORD WINAPI MarkerThread(LPVOID param)
{
    MarkerThreadData* data = (MarkerThreadData*)param;
    if (!data) return 1;

    WaitForSingleObject(data->startEvent, INFINITE);
    srand(data->threadId);

    int markedCount = 0;
    bool running = true;
    int lastIndex = -1;

    while (running)
    {
        int index = rand() % data->arraySize;
        EnterCriticalSection(data->cs);

        if (data->sharedArray[index] == 0)
        {
            Sleep(5);
            data->sharedArray[index] = data->threadId;
            Sleep(5);
            markedCount++;
            LeaveCriticalSection(data->cs);
        }
        else {
            lastIndex = index;
            LeaveCriticalSection(data->cs);

            EnterCriticalSection(data->outputCs);
            std::cout << "Поток " << data->threadId << " отметил " << markedCount
                << " элементов. Остановлен на индексе " << lastIndex << std::endl;
            LeaveCriticalSection(data->outputCs);

            if (data->suspendEvent) {
                SetEvent(data->suspendEvent);
            }

            HANDLE waitHandles[2] = { data->terminateEvent, data->continueEvent };
            DWORD waitResult = WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);

            if (waitResult == WAIT_OBJECT_0)
            {
                running = false;
            }
            else if (waitResult == WAIT_OBJECT_0 + 1)
            {
                markedCount = 0;
                lastIndex = -1;
                continue;
            }
            break;
        }

        if (data->terminateEvent && WaitForSingleObject(data->terminateEvent, 0) == WAIT_OBJECT_0)
        {
            running = false;
        }
    }

    if (!running)
    {
        EnterCriticalSection(data->cs);
        int cleanedCount = 0;
        for (int i = 0; i < data->arraySize; ++i)
        {
            if (data->sharedArray[i] == data->threadId)
            {
                data->sharedArray[i] = 0;
                cleanedCount++;
            }
        }
        LeaveCriticalSection(data->cs);

        EnterCriticalSection(data->outputCs);
        std::cout << "Поток " << data->threadId << " завершен. Очищено "
            << cleanedCount << " элементов." << std::endl;
        LeaveCriticalSection(data->outputCs);
    }

    return 0;
}