#include <iostream>
#include <string>
#include <vector>
#include <algorithm> // Для individual variant
#include <sstream>   // Для individual variant
#include <windows.h>
#include <ctime>

using namespace std;

// --- Имена именованных объектов синхронизации (должны совпадать) ---
const char* SEMAPHORE_NAME = "DownloadSlots";
const char* MUTEX_NAME = "LogAccessMutex";
const char* EVENT_NAME = "BrowserClosingEvent";

// --- Вспомогательная функция для логирования (защищена мьютексом) ---
void Log(HANDLE hMutex, const string& message) {
    // Захват мьютекса
    DWORD waitResult = WaitForSingleObject(hMutex, INFINITE);
    if (waitResult == WAIT_OBJECT_0) {
        // Запись в лог (консоль)
        cout << message << endl;
        // Освобождение мьютекса
        ReleaseMutex(hMutex);
    }
    else {
        // Если захватить не удалось (ошибка)
        cerr << "[PID: " << GetCurrentProcessId() << "] ERROR: Failed to acquire Log Mutex. Message lost: " << message << endl;
    }
}

// --- Индивидуальный вариант №6: Найти самую длинную строку в массиве строк ---
void ProcessFile(const string& fileName) {
    // Имитация данных (массив строк)
    vector<string> data = {
        "short",
        "This is a medium length string.",
        "A bit longer than the medium one.",
        "The very very very longest string in the entire array of data for processing. Wow.",
        "last"
    };

    // Выполнение задачи
    size_t maxLength = 0;
    string longestString;

    for (const auto& str : data) {
        if (str.length() > maxLength) {
            maxLength = str.length();
            longestString = str;
        }
    }

    // Имитация результата обработки
    // В реальном приложении здесь было бы сохранение результата
    Log(GetStdHandle(STD_OUTPUT_HANDLE),
        "[PID: " + to_string(GetCurrentProcessId()) + "] **Processing Done:** Longest string length is " +
        to_string(maxLength) + " for file '" + fileName + "'.");
}


int main(int argc, char* argv[]) {
    // Получение PID для сообщений
    DWORD pid = GetCurrentProcessId();

    if (argc < 2) {
        cerr << "[PID: " << pid << "] Error: Downloader requires file name as argument. Exiting." << endl;
        return 1;
    }

    string fileName = argv[1];
    string uniqueId = (argc > 2) ? argv[2] : "N/A"; // Уникальный ID для трассировки

    // 1. "Подключение" к среде браузера (открытие именованных объектов)
    // Открытие семафора
    HANDLE hSemaphore = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, FALSE, SEMAPHORE_NAME);
    if (hSemaphore == NULL) {
        cerr << "[PID: " << pid << "] Error: Failed to open Semaphore '" << SEMAPHORE_NAME << "'. Code: " << GetLastError() << endl;
        return 1;
    }

    // Открытие мьютекса
    HANDLE hMutex = OpenMutexA(MUTEX_ALL_ACCESS, FALSE, MUTEX_NAME);
    if (hMutex == NULL) {
        cerr << "[PID: " << pid << "] Error: Failed to open Mutex '" << MUTEX_NAME << "'. Code: " << GetLastError() << endl;
        CloseHandle(hSemaphore);
        return 1;
    }

    // Открытие события
    HANDLE hEvent = OpenEventA(EVENT_ALL_ACCESS, FALSE, EVENT_NAME);
    if (hEvent == NULL) {
        cerr << "[PID: " << pid << "] Error: Failed to open Event '" << EVENT_NAME << "'. Code: " << GetLastError() << endl;
        CloseHandle(hSemaphore);
        CloseHandle(hMutex);
        return 1;
    }

    Log(hMutex, "[PID: " + to_string(pid) + "] Process #" + uniqueId + " started for '" + fileName + "'. Resources opened.");

    // --- Шаг 1: Ожидание в очереди (Семафор или Событие закрытия) ---
    HANDLE handles[] = { hSemaphore, hEvent }; // hSemaphore - индекс 0, hEvent - индекс 1

    Log(hMutex, "[PID: " + to_string(pid) + "] Process #" + uniqueId + " waiting for a download slot...");

    DWORD waitResult = WaitForMultipleObjects(
        2,        // Количество объектов для ожидания
        handles,  // Массив дескрипторов
        FALSE,    // Ждать ЛЮБОГО объекта (Wait Any)
        INFINITE  // Ждать бесконечно
    );

    // Проверка результата ожидания
    if (waitResult == WAIT_OBJECT_0) { // Сработал Семафор (hSemaphore)
        // --- Шаг 2: Начало загрузки ---

        // 4a, 4b, 4c: Захват мьютекса и логирование
        Log(hMutex, "[PID: " + to_string(pid) + "] Slot acquired. Connection established. Starting download of '" + fileName + "'...");

        // 4d: Имитация "обработки" скачанного файла
        ProcessFile(fileName);

        // 4e: Имитация времени загрузки (1-3 секунды)
        srand(pid * time(0)); // Установка seed для рандомизации
        int sleepTime = (rand() % 3 + 1) * 1000; // 1000, 2000 или 3000 мс
        Sleep(sleepTime);

        // 4f, 4g, 4h: Логирование завершения
        Log(hMutex, "[PID: " + to_string(pid) + "] File '" + fileName + "' processed successfully in " + to_string(sleepTime / 1000) + "s.");

        // --- Шаг 3: Завершение (Успешно) ---
        // Освобождение семафора (слота)
        ReleaseSemaphore(hSemaphore, 1, NULL);
        Log(hMutex, "[PID: " + to_string(pid) + "] Slot released. Task finished.");

    }
    else if (waitResult == (WAIT_OBJECT_0 + 1)) { // Сработало Событие (hEvent)
        // --- Шаг 3: Завершение (Прервано) ---
        // Браузер закрывается - прерываем работу. Не освобождаем слот, т.к. не занимали.
        Log(hMutex, "[PID: " + to_string(pid) + "] Received **BrowserClosingEvent**. Download of '" + fileName + "' interrupted.");

    }
    else {
        // Ошибка ожидания
        Log(hMutex, "[PID: " + to_string(pid) + "] Error waiting for slot or close signal. Code: " + to_string(GetLastError()) + ". Exiting.");
    }

    // Корректное закрытие дескрипторов
    CloseHandle(hSemaphore);
    CloseHandle(hMutex);
    CloseHandle(hEvent);

    return 0;
}
