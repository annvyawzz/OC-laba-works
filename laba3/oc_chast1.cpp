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
            cout << "Поток " << data->threadId << " отметил " << markedCount
                << " элементов. Остановлен на индексе " << lastIndex << endl;

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

        cout << "Поток " << data->threadId << " завершен. Очищено "
            << cleanedCount << " элементов." << endl;
    }

    return 0;
}

int main() 
{
    setlocale(LC_ALL, "RUS");
    int arraySize;
    cout << "Введите размер массива: ";
    cin >> arraySize;

    int* sharedArray = new int[arraySize](); 

    int markerCount;
    cout << "Введите количество потоков marker: ";
    cin >> markerCount;

    CRITICAL_SECTION cs;
    InitializeCriticalSection(&cs);

    // b. Массив событий для приостановки
    HANDLE* suspendEvents = new HANDLE[markerCount];
    // c. Массив событий для завершения
    HANDLE* terminateEvents = new HANDLE[markerCount];
    // d. Массив событий для продолжения
    HANDLE* continueEvents = new HANDLE[markerCount];
    // e. Событие для одновременного старта
    HANDLE startEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    for (int i = 0; i < markerCount; ++i) 
    {
        suspendEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL); 
        terminateEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL); 
        continueEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL); 
    }

    // Шаг 4: Запуск потоков marker
    HANDLE* markerThreads = new HANDLE[markerCount];
    MarkerThreadData* threadData = new MarkerThreadData[markerCount];

    for (int i = 0; i < markerCount; ++i)
    {
        threadData[i] = {
            i + 1,             
            arraySize,          
            sharedArray,       
            &cs,             
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

        if (markerThreads[i] == NULL)
        {
            cerr << "ЕРРОР" << (i + 1) << endl;
            return GetLastError();
        }
    }

    cout << "Создано " << markerCount << " потоков marker" << endl;

    SetEvent(startEvent);
    cout << "Все потоки запущены одновременно" << endl;

    // Шаг 6: Главный управляющий цикл
    vector<bool> activeMarkers(markerCount, true);
    int activeCount = markerCount;

    while (activeCount > 0) 
    {
        // Шаг 6a: Ожидание приостановки всех активных потоков
        cout << "\n Ожидание приостановки всех активных потоков..." << endl;
        WaitForMultipleObjects(markerCount, suspendEvents, TRUE, INFINITE);

        // Шаг 6b: Вывод тек состояния массива
        EnterCriticalSection(&cs);
        cout << "Текущее состояние массива: ";
        for (int i = 0; i < arraySize; ++i)
        {
            cout << sharedArray[i] << " ";
        }
        cout << endl;
        LeaveCriticalSection(&cs);

        // запрос номера потока для завершения
        int threadToTerminate;
        cout << "Введите номер потока marker для завершения(1 -"
            << markerCount << "): ";
        cin >> threadToTerminate;

        if (threadToTerminate < 1 || threadToTerminate > markerCount ||
            !activeMarkers[threadToTerminate - 1])
        {
            cout << "Неверный номер потока! AAAAAAAAAAAAAA" << endl;
            continue;
        }

        int threadIndex = threadToTerminate - 1;

        // Шаг 6d: Сигнал на завершение выбранному потоку
        cout << "Завершение потока" << threadToTerminate << "..." << endl;
        SetEvent(terminateEvents[threadIndex]);

        // ожидание завершения потока
        WaitForSingleObject(markerThreads[threadIndex], INFINITE);
        CloseHandle(markerThreads[threadIndex]);
        activeMarkers[threadIndex] = false;
        activeCount--;

        // вывод состояния массива после очистки
        EnterCriticalSection(&cs);
        cout << "cостояние массива после завершения потока" << threadToTerminate << ": ";
        for (int i = 0; i < arraySize; ++i)
        {
            cout << sharedArray[i] << " ";
        }
        cout << endl;
        LeaveCriticalSection(&cs);

        // сигнал на продолжение оставшимся потокам
        if (activeCount > 0) 
        {
            cout << "Возобновление работы оставшихся " << activeCount << " потоков..." << endl;
            for (int i = 0; i < markerCount; ++i) {
                if (activeMarkers[i]) {
                    SetEvent(continueEvents[i]);
                }
            }
        }

        cout << "Активных потоков осталось: " << activeCount << endl;
    }

    cout << "\n Все потоки marker завершены." << endl;

    DeleteCriticalSection(&cs);

    CloseHandle(startEvent);
    for (int i = 0; i < markerCount; ++i)
    {
        CloseHandle(suspendEvents[i]);
        CloseHandle(terminateEvents[i]);
        CloseHandle(continueEvents[i]);
    }

    delete[] sharedArray;
    delete[] suspendEvents;
    delete[] terminateEvents;
    delete[] continueEvents;
    delete[] markerThreads;
    delete[] threadData;

    cout << "Программа завершена успешно.УРАУРАУРА happy happy Нажмите Enter для выхода." << endl;
    cin.ignore();
    cin.get();

    return 0;
}
