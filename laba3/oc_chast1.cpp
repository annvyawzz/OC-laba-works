#include <windows.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

using namespace std;

struct MarkerThreadData
{
    int threadId;           // номер потока
    int arraySize;         
    int* sharedArray;       // указатель на общий массив
    CRITICAL_SECTION* cs;   // указатель крит секции
    HANDLE startEvent;      // событие для одновременного старта
    HANDLE suspendEvent;    // приостановка
    HANDLE terminateEvent;  // завершение
    HANDLE continueEvent;   // прода
};

//поток-marker
DWORD WINAPI MarkerThread(LPVOID param) 
{
    MarkerThreadData* data = (MarkerThreadData*)param;
    // шаг1: Ожидание сигнала старта от main
    WaitForSingleObject(data->startEvent, INFINITE);

    // шаг2: Инициализация генератора случайных чисел
    srand(data->threadId);

    int markedCount = 0;
    bool running = true;
    int lastIndex = -1; 

    while (running)
    {
        // генерация случайного индекса
        int index = rand() % data->arraySize;

        //захват крит секции
        EnterCriticalSection(data->cs);

        //проверка ячейки
        if (data->sharedArray[index] == 0)
        {
            // ячейка свободна - ставим метку
            Sleep(5); 
            data->sharedArray[index] = data->threadId;
            markedCount++;
            Sleep(5); 
            LeaveCriticalSection(data->cs);
        }
        else {
            // ячейка занята - запоминаем индекс и выход
            lastIndex = index;
            LeaveCriticalSection(data->cs);

            // шаг4: Обработка приостановки
            cout << "Thread " << data->threadId << " marked " << markedCount
                << " elements. Stopped at index " << lastIndex << endl;

            SetEvent(data->suspendEvent);

            // Шаг 4b: Ожидание команды от main
            HANDLE waitHandles[2] = { data->terminateEvent, data->continueEvent };
            DWORD waitResult = WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);

            if (waitResult == WAIT_OBJECT_0)
            {
                running = false;
            }
            else if (waitResult == WAIT_OBJECT_0 + 1)
            {
                markedCount = 0; // Сброс счетчика для нового цикла
                continue;
            }
        }
    }

    // Шаг 5: Завершение работы (если running = false)
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

        cout << "Thread " << data->threadId << " finished. Cleaned "
            << cleanedCount << " elements." << endl;
    }

    return 0;
}
