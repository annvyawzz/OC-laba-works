#include <windows.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

using namespace std;

struct MarkerThreadData
{
    int threadId;           
    int arraySize;
    int* sharedArray;       
    CRITICAL_SECTION* cs;   // указатель крит секции
    CRITICAL_SECTION* outputCs; // для синхронизации вывода
    HANDLE startEvent;      // событие для одновременного старта
    HANDLE suspendEvent;    // приостановка
    HANDLE terminateEvent;  // завершение
    HANDLE continueEvent;   // продолжение
};

// поток-marker
DWORD WINAPI MarkerThread(LPVOID param)
{
    MarkerThreadData* data = (MarkerThreadData*)param;

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
            // Успешная отметка
            Sleep(5);
            data->sharedArray[index] = data->threadId;
            Sleep(5);
            markedCount++;
            LeaveCriticalSection(data->cs);
        }
        else {
            // Неудачная попытка - выходим из цикла
            lastIndex = index;
            LeaveCriticalSection(data->cs);

            // Сообщаем о приостановке
            EnterCriticalSection(data->outputCs);
            cout << "Поток " << data->threadId << " отметил " << markedCount
                << " элементов. Остановлен на индексе " << lastIndex << endl;
            LeaveCriticalSection(data->outputCs);

            SetEvent(data->suspendEvent);

            // Ожидание команды от main
            HANDLE waitHandles[2] = { data->terminateEvent, data->continueEvent };
            DWORD waitResult = WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);

            if (waitResult == WAIT_OBJECT_0)
            {
                running = false; // Завершение
            }
            else if (waitResult == WAIT_OBJECT_0 + 1)
            {
                // Продолжение работы
                markedCount = 0;
                lastIndex = -1;
                continue;
            }
            break;
        }

        // Проверка сигнала завершения
        if (WaitForSingleObject(data->terminateEvent, 0) == WAIT_OBJECT_0)
        {
            running = false;
        }
    }

    // Завершение работы - очистка массива
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
        cout << "Поток " << data->threadId << " завершен. Очищено "
            << cleanedCount << " элементов." << endl;
        LeaveCriticalSection(data->outputCs);
    }

    return 0;
}

int main()
{
    setlocale(LC_ALL, "RUS");

    // Шаг 1: Создание и инициализация массива
    int arraySize;
    cout << "Введите размер массива: ";
    cin >> arraySize;

    int* sharedArray = new int[arraySize]();

    // Шаг 2: Запрос количества потоков
    int markerCount;
    cout << "Введите количество потоков marker: ";
    cin >> markerCount;

    // Шаг 3: Создание объектов синхронизации
    CRITICAL_SECTION cs;
    CRITICAL_SECTION outputCs;
    InitializeCriticalSection(&cs);
    InitializeCriticalSection(&outputCs);

    HANDLE* suspendEvents = new HANDLE[markerCount];
    HANDLE* terminateEvents = new HANDLE[markerCount];
    HANDLE* continueEvents = new HANDLE[markerCount];
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

        if (markerThreads[i] == NULL)
        {
            cerr << "Ошибка создания потока " << (i + 1) << endl;
            return GetLastError();
        }
    }

    cout << "Создано " << markerCount << " потоков marker" << endl;

    // Шаг 5: Сигнал на начало работы
    SetEvent(startEvent);
    cout << "Все потоки запущены одновременно" << endl;

    // Шаг 6: Главный управляющий цикл
    vector<bool> activeMarkers(markerCount, true);
    int activeCount = markerCount;

    while (activeCount > 0)
    {
        // Шаг 6a: Ожидание приостановки одного потока
        cout << "\nОжидание приостановки одного из потоков..." << endl;

        // Создаем массив активных событий приостановки
        vector<HANDLE> activeSuspendEvents;
        vector<int> activeIndices;

        for (int i = 0; i < markerCount; ++i) {
            if (activeMarkers[i]) {
                activeSuspendEvents.push_back(suspendEvents[i]);
                activeIndices.push_back(i);
            }
        }

        if (activeSuspendEvents.empty()) break;

        // Ждем любое из событий приостановки
        DWORD waitResult = WaitForMultipleObjects(
            activeSuspendEvents.size(),
            &activeSuspendEvents[0],
            FALSE,
            INFINITE
        );

        if (waitResult == WAIT_FAILED) {
            cout << "Ошибка при ожидании событий" << endl;
            break;
        }

        int eventIndex = waitResult - WAIT_OBJECT_0;
        if (eventIndex < 0 || eventIndex >= activeIndices.size()) {
            continue;
        }

        int suspendedThreadIndex = activeIndices[eventIndex];

        // Небольшая задержка для завершения вывода потоков
        Sleep(50);

        // Шаг 6b: Вывод текущего состояния массива
        EnterCriticalSection(&cs);
        cout << "Текущее состояние массива: ";
        for (int i = 0; i < arraySize; ++i) {
            cout << sharedArray[i] << " ";
        }
        cout << endl;
        LeaveCriticalSection(&cs);

        // Шаг 6c: Запрос номера потока для завершения
        int threadToTerminate;
        cout << "Введите номер потока marker для завершения (1-"
            << markerCount << "): ";
        cin >> threadToTerminate;

        // Проверка корректности ввода
        if (threadToTerminate < 1 || threadToTerminate > markerCount ||
            !activeMarkers[threadToTerminate - 1]) {
            cout << "Неверный номер потока! Попробуйте снова." << endl;

            // Продолжаем все потоки
            for (int i = 0; i < markerCount; ++i) {
                if (activeMarkers[i]) {
                    SetEvent(continueEvents[i]);
                }
            }
            continue;
        }

        int threadIndex = threadToTerminate - 1;

        // Шаг 6d: Завершение выбранного потока
        cout << "Завершение потока " << threadToTerminate << "..." << endl;
        SetEvent(terminateEvents[threadIndex]);
        WaitForSingleObject(markerThreads[threadIndex], INFINITE);
        CloseHandle(markerThreads[threadIndex]);
        activeMarkers[threadIndex] = false;
        activeCount--;

        // Шаг 6e: Вывод состояния после очистки
        EnterCriticalSection(&cs);
        cout << "Состояние массива после завершения потока " << threadToTerminate << ": ";
        for (int i = 0; i < arraySize; ++i) {
            cout << sharedArray[i] << " ";
        }
        cout << endl;
        LeaveCriticalSection(&cs);

        // Шаг 6f: Продолжение работы оставшихся потоков
        if (activeCount > 0) {
            cout << "Возобновление работы оставшихся " << activeCount << " потоков..." << endl;
            for (int i = 0; i < markerCount; ++i) {
                if (activeMarkers[i]) {
                    SetEvent(continueEvents[i]);
                }
            }
        }

        cout << "Активных потоков осталось: " << activeCount << endl;
    }

    cout << "\nВсе потоки marker завершены!" << endl;

    DeleteCriticalSection(&cs);
    DeleteCriticalSection(&outputCs);
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

    cout << "Программа завершена успешно!" << endl;
    cout << "Нажмите Enter для выхода...";
    cin.ignore();
    cin.get();

    return 0;
}
