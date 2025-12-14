#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <windows.h>
#include <ctime>

using namespace std;

const char* SEMAPHORE_NAME = "DownloadSlots_NEW_2024";
const char* MUTEX_NAME = "LogAccessMutex_NEW_2024";
const char* EVENT_NAME = "BrowserClosingEvent_NEW_2024";

void Log(HANDLE hMutex, const string& message)
{
    DWORD waitResult = WaitForSingleObject(hMutex, INFINITE);

    if (waitResult == WAIT_OBJECT_0)
    {
        SYSTEMTIME st;
        GetLocalTime(&st);
        char timeStr[64];
        sprintf_s(timeStr, "%02d:%02d:%02d.%03d",
            st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

        cout << "[" << timeStr << "] " << message << endl;
        ReleaseMutex(hMutex);
    }
    else {
        cerr << "[ERROR] Не удалось захватить мьютекс для логирования!" << endl;
    }
}

void ProcessFile(HANDLE hMutex, const string& fileName)
{
    DWORD pid = GetCurrentProcessId();

    vector<string> data = {
        "короткая строка",
        "Это строка средней длины.",
        "Немного длиннее, чем средняя.",
        "Самая самая самая длинная строка в этом массиве. Вот это да пам пам пам парам !",
        "последняя строка в списке",
        "Еще одна дополнительная строка",
        "Строка с цифрами: 1234567890",
        "Просто текст",
        "И еще немного текста для разнообразия",
        "Финальная строка в наборе"
    };

    data.push_back("Имя обрабатываемого файла: " + fileName);

    Log(hMutex, "[PID: " + to_string(pid) +
        "] Начинаю обработку файла: " + fileName);
    Log(hMutex, "[PID: " + to_string(pid) +
        "] Всего строк для анализа: " + to_string(data.size()));

    size_t maxLength = 0;
    string longestString;
    int longestIndex = -1;

    for (size_t i = 0; i < data.size(); i++) {
        if (data[i].length() > maxLength) {
            maxLength = data[i].length();
            longestString = data[i];
            longestIndex = static_cast<int>(i);
        }
    }

    stringstream result;
    result << "[PID: " << pid << "] **РЕЗУЛЬТАТ ОБРАБОТКИ**" << endl;
    result << "  Файл: " << fileName << endl;
    result << "  Самая длинная строка (индекс " << longestIndex << "):" << endl;
    result << "  \"" << longestString << "\"" << endl;
    result << "  Длина: " << maxLength << " символов";

    Log(hMutex, result.str());
}

int main(int argc, char* argv[])
{
    setlocale(LC_ALL, "rus");
    DWORD pid = GetCurrentProcessId();

    if (argc < 2) {
        cerr << "[PID: " << pid << "] Ошибка: Downloader требует имя файла в качестве аргумента. Выход" << endl;
        return 1;
    }

    string fileName = argv[1];
    string uniqueId = (argc > 2) ? argv[2] : "N/A";

    HANDLE hSemaphore = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, FALSE, SEMAPHORE_NAME);
    if (hSemaphore == NULL) {
        cerr << "[PID: " << pid << "] Ошибка: Не удалось открыть Семафор '" << SEMAPHORE_NAME << "'. Код: " << GetLastError() << endl;
        return 1;
    }

    HANDLE hMutex = OpenMutexA(MUTEX_ALL_ACCESS, FALSE, MUTEX_NAME);
    if (hMutex == NULL) {
        cerr << "[PID: " << pid << "] Ошибка: Не удалось открыть Мьютекс '" << MUTEX_NAME << "'. Код: " << GetLastError() << endl;
        CloseHandle(hSemaphore);
        return 1;
    }

    HANDLE hEvent = OpenEventA(EVENT_ALL_ACCESS, FALSE, EVENT_NAME);
    if (hEvent == NULL) {
        ULONGLONG tickCount = GetTickCount64() % 1000;
        string altEventName = string(EVENT_NAME) + "_" + to_string(tickCount);
        hEvent = OpenEventA(EVENT_ALL_ACCESS, FALSE, altEventName.c_str());

        if (hEvent == NULL) {
            cerr << "[PID: " << pid << "] Ошибка: Не удалось открыть Событие. Код: " << GetLastError() << endl;
            CloseHandle(hSemaphore);
            CloseHandle(hMutex);
            return 1;
        }
    }

    Log(hMutex, "[PID: " + to_string(pid) + "] Процесс #" + uniqueId +
        " запущен для файла '" + fileName + "'. Ресурсы открыты.");

    HANDLE handles[] = { hSemaphore, hEvent };

    Log(hMutex, "[PID: " + to_string(pid) + "] Процесс #" + uniqueId +
        " ожидает слот загрузки...");

    DWORD waitResult = WaitForMultipleObjects(
        2,
        handles,
        FALSE,
        INFINITE
    );

    if (waitResult == WAIT_OBJECT_0)
    {
        Log(hMutex, "[PID: " + to_string(pid) + "] Слот получен! Начинаю загрузку файла '" + fileName + "'...");
        Log(hMutex, "[PID: " + to_string(pid) + "] Connection established. Starting download of '" + fileName + "'...");

        ProcessFile(hMutex, fileName);

        srand(static_cast<unsigned int>(pid * time(NULL)));
        int sleepTime = (rand() % 3 + 1) * 1000;
        Sleep(static_cast<DWORD>(sleepTime));

        Log(hMutex, "[PID: " + to_string(pid) + "] File '" + fileName +
            "' processed successfully. Время обработки: " +
            to_string(sleepTime / 1000.0) + " сек.");

        if (!ReleaseSemaphore(hSemaphore, 1, NULL)) {
            Log(hMutex, "[PID: " + to_string(pid) + "] Ошибка освобождения слота: " + to_string(GetLastError()));
        }
        else {
            Log(hMutex, "[PID: " + to_string(pid) + "] Слот освобожден. Задача завершена.");
        }
    }
    else if (waitResult == (WAIT_OBJECT_0 + 1))
    {
        Log(hMutex, "[PID: " + to_string(pid) + "] Получен **BrowserClosingEvent**. Загрузка '" +
            fileName + "' прервана.");
    }
    else
    {
        DWORD error = GetLastError();
        Log(hMutex, "[PID: " + to_string(pid) + "] Ошибка при ожидании. Код: " + to_string(error));
    }

    if (hSemaphore != NULL) CloseHandle(hSemaphore);
    if (hMutex != NULL) CloseHandle(hMutex);
    if (hEvent != NULL) CloseHandle(hEvent);

    return 0;
}
