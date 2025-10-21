#include <iostream>
#include <windows.h>
#include <vector>
#include <process.h>

using namespace std;

// Структура для передачи данных в поток marker
struct MarkerData {
    int markerId;
    int arraySize;
    int* array;
    CRITICAL_SECTION* cs;
    HANDLE startEvent;
    HANDLE stopEvent;
    HANDLE continueEvent;
    HANDLE threadFinishedEvent;
};

// Функция потока marker
unsigned __stdcall markerThread(void* param) {
    MarkerData* data = (MarkerData*)param;

    // Ожидаем сигнала начала работы
    WaitForSingleObject(data->startEvent, INFINITE);

    srand(data->markerId); // Инициализируем генератор случайных чисел

    int markedCount = 0;
    int lastIndex = -1;

    while (true) {
        int index = rand() % data->arraySize;

        // Входим в критическую секцию
        EnterCriticalSection(data->cs);

        if (data->array[index] == 0) {
            // Ячейка свободна - помечаем её
            Sleep(5);
            data->array[index] = data->markerId;
            Sleep(5);
            markedCount++;
            lastIndex = index;

            // Выходим из критической секции
            LeaveCriticalSection(data->cs);
        }
        else {
            // Ячейка занята - выходим из цикла
            LeaveCriticalSection(data->cs);
            break;
        }
    }

    // Сообщаем о невозможности продолжать работу
    cout << "Поток " << data->markerId << " остановился. Помечено ячеек: "
        << markedCount << ". Последняя ячейка: " << lastIndex << endl;

    SetEvent(data->threadFinishedEvent);

    // Ждем сигнала от main
    HANDLE events[2] = { data->continueEvent, data->stopEvent };
    DWORD result = WaitForMultipleObjects(2, events, FALSE, INFINITE);

    if (result == WAIT_OBJECT_0 + 1) {
        // Получили сигнал на завершение - очищаем свои отметки
        EnterCriticalSection(data->cs);

        for (int i = 0; i < data->arraySize; i++) {
            if (data->array[i] == data->markerId) {
                data->array[i] = 0;
            }
        }

        LeaveCriticalSection(data->cs);
    }

    // Завершаем поток
    _endthreadex(0);
    return 0;
}

// Функция для вывода массива
void printArray(int* array, int size) {
    cout << "Текущее состояние массива: ";
    for (int i = 0; i < size; i++) {
        cout << array[i] << " ";
    }
    cout << endl;
}

int main() {
    setlocale(LC_ALL, "Russian");

    // 1. Создаем и инициализируем массив
    int arraySize;
    cout << "Введите размер массива: ";
    cin >> arraySize;

    int* array = new int[arraySize];
    for (int i = 0; i < arraySize; i++) {
        array[i] = 0;
    }

    // 2. Запрашиваем количество потоков
    int threadCount;
    cout << "Введите количество потоков marker: ";
    cin >> threadCount;

    // 3. Создаем объекты синхронизации
    CRITICAL_SECTION cs;
    InitializeCriticalSection(&cs);

    HANDLE startEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    vector<HANDLE> threadFinishedEvents(threadCount);
    vector<HANDLE> continueEvents(threadCount);
    vector<HANDLE> stopEvents(threadCount);
    vector<HANDLE> threads(threadCount);
    vector<MarkerData*> threadData(threadCount);

    for (int i = 0; i < threadCount; i++) {
        threadFinishedEvents[i] = CreateEvent(NULL, TRUE, FALSE, NULL);
        continueEvents[i] = CreateEvent(NULL, TRUE, FALSE, NULL);
        stopEvents[i] = CreateEvent(NULL, TRUE, FALSE, NULL);
    }

    // 4. Запускаем потоки marker
    for (int i = 0; i < threadCount; i++) {
        threadData[i] = new MarkerData{
            i + 1,           // markerId (начинаем с 1)
            arraySize,       // arraySize
            array,           // array
            &cs,             // cs
            startEvent,      // startEvent
            stopEvents[i],   // stopEvent
            continueEvents[i], // continueEvent
            threadFinishedEvents[i] // threadFinishedEvent
        };

        threads[i] = (HANDLE)_beginthreadex(NULL, 0, markerThread, threadData[i], 0, NULL);
    }

    // 5. Даем общий сигнал на начало работы
    SetEvent(startEvent);

    // 6. Главный управляющий цикл
    int activeThreads = threadCount;

    while (activeThreads > 0) {
        // 6a. Ждем, пока все активные потоки сообщат о невозможности продолжать работу
        WaitForMultipleObjects(activeThreads, threadFinishedEvents.data(), TRUE, INFINITE);

        // 6b. Выводим текущее состояние массива
        printArray(array, arraySize);

        // 6c. Запрашиваем номер потока для завершения
        int threadToStop;
        cout << "Введите номер потока для завершения (1-" << threadCount << "): ";
        cin >> threadToStop;

        if (threadToStop < 1 || threadToStop > threadCount) {
            cout << "Неверный номер потока!" << endl;
            continue;
        }

        int threadIndex = threadToStop - 1;

        // 6d. Подаем сигнал на завершение выбранному потоку
        SetEvent(stopEvents[threadIndex]);

        // 6e. Ждем завершения потока
        WaitForSingleObject(threads[threadIndex], INFINITE);

        // 6f. Выводим состояние массива после очистки
        cout << "После завершения потока " << threadToStop << ":" << endl;
        printArray(array, arraySize);

        // Убираем завершенный поток из активных
        threadFinishedEvents.erase(threadFinishedEvents.begin() + threadIndex);
        activeThreads--;

        // 6g. Подаем сигнал на продолжение работы оставшимся потокам
        for (int i = 0; i < threadCount; i++) {
            if (i != threadIndex) {
                ResetEvent(threadFinishedEvents[i]);
                SetEvent(continueEvents[i]);
            }
        }
    }

    // Освобождаем ресурсы
    DeleteCriticalSection(&cs);
    CloseHandle(startEvent);

    for (int i = 0; i < threadCount; i++) {
        CloseHandle(threads[i]);
        CloseHandle(continueEvents[i]);
        CloseHandle(stopEvents[i]);
        CloseHandle(threadFinishedEvents[i]);
        delete threadData[i];
    }

    delete[] array;

    cout << "Все потоки завершены. Программа завершена." << endl;

    return 0;
}