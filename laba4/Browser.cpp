#include <iostream>
#include <vector>
#include <string>
#include <windows.h>
#include <ctime>

using namespace std;

// --- Имена именованных объектов синхронизации ---
const char* SEMAPHORE_NAME = "DownloadSlots";
const char* MUTEX_NAME = "LogAccessMutex";
const char* EVENT_NAME = "BrowserClosingEvent";

int main() {
    // 1. Запрос параметров
    int N_slots;
    int M_downloads;

    cout << "--- Browser.exe Initialization ---" << endl;
    cout << "Enter max simultaneous downloads (N): ";
    cin >> N_slots;
    cout << "Enter total files to download (M, must be > N): ";
    cin >> M_downloads;

    if (M_downloads <= N_slots) {
        cerr << "Error: M must be greater than N. Exiting." << endl;
        return 1;
    }

    // 2. Создание именованных объектов синхронизации
    cout << "\n2. Creating synchronization objects..." << endl;

    // Семафор: DownloadSlots (Начальное/Макс. значение = N)
    HANDLE hSemaphore = CreateSemaphoreA(
        NULL,             // Атрибуты безопасности по умолчанию
        N_slots,          // Начальное количество
        N_slots,          // Максимальное количество
        SEMAPHORE_NAME    // Имя семафора
    );
    if (hSemaphore == NULL) {
        cerr << "Error creating Semaphore: " << GetLastError() << endl;
        return 1;
    }
    cout << "Semaphore '" << SEMAPHORE_NAME << "' created with N=" << N_slots << "." << endl;

    // Мьютекс: LogAccessMutex
    HANDLE hMutex = CreateMutexA(
        NULL,             // Атрибуты безопасности по умолчанию
        FALSE,            // Изначально не захвачен
        MUTEX_NAME        // Имя мьютекса
    );
    if (hMutex == NULL) {
        cerr << "Error creating Mutex: " << GetLastError() << endl;
        CloseHandle(hSemaphore);
        return 1;
    }
    cout << "Mutex '" << MUTEX_NAME << "' created." << endl;

    // Событие: BrowserClosingEvent (Ручной сброс, Изначально несигнальное)
    HANDLE hEvent = CreateEventA(
        NULL,             // Атрибуты безопасности по умолчанию
        TRUE,             // Ручной сброс (Manual-reset)
        FALSE,            // Изначально несигнальное (Non-signaled)
        EVENT_NAME        // Имя события
    );
    if (hEvent == NULL) {
        cerr << "Error creating Event: " << GetLastError() << endl;
        CloseHandle(hSemaphore);
        CloseHandle(hMutex);
        return 1;
    }
    cout << "Event '" << EVENT_NAME << "' created (Manual-reset, Initial: Non-signaled)." << endl;

    // 3. Запуск M экземпляров Downloader.exe
    vector<HANDLE> downloader_handles;
    cout << "\n3. Launching " << M_downloads << " instances of Downloader.exe..." << endl;

    for (int i = 0; i < M_downloads; ++i) {
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        // Имя файла для имитации (для индивидуального варианта)
        string fileName = "file_" + to_string(i + 1) + ".txt";

        string commandLine = "Downloader.exe " + fileName + " " + to_string(i + 1);

        if (!CreateProcessA(
            NULL,                       // Модуль (можно NULL, если путь в командной строке)
            (LPSTR)commandLine.c_str(), // Командная строка (должна быть не-const)
            NULL,                       // Атрибуты безопасности процесса
            NULL,                       // Атрибуты безопасности потока
            FALSE,                      // Не наследовать дескрипторы
            CREATE_NEW_CONSOLE,         // Создать новое консольное окно
            NULL,                       // Использовать окружение родителя
            NULL,                       // Использовать текущую директорию
            &si,                        // Информация о запуске
            &pi                         // Информация о процессе
        )) {
            cerr << "Error launching Downloader #" << i + 1 << ": " << GetLastError() << endl;
            continue;
        }

        // Сохраняем дескриптор процесса для ожидания
        downloader_handles.push_back(pi.hProcess);

        // Закрываем ненужный дескриптор потока
        CloseHandle(pi.hThread);
    }
    cout << M_downloads << " instances launched." << endl;

    // 4. Ожидание нажатия Enter
    cout << "\n--- Browser is running. Press **Enter** to close... ---" << endl;
    cin.ignore(); // Очистка буфера после cin >> M_downloads
    cin.get();    // Ожидание нажатия Enter

    // 5. Завершение работы
    cout << "\n5a. Browser is closing. Sending termination signal to all downloads..." << endl;

    // 5b. Переводим событие в сигнальное состояние
    if (!SetEvent(hEvent)) {
        cerr << "Error setting BrowserClosingEvent: " << GetLastError() << endl;
    }
    else {
        cout << "Termination signal **SENT**." << endl;
    }

    // 6. Дождаться завершения всех Downloader'ов
    cout << "6. Waiting for all Downloader processes to terminate..." << endl;
    if (!downloader_handles.empty()) {
        WaitForMultipleObjects(
            downloader_handles.size(),  // Количество дескрипторов
            downloader_handles.data(),  // Массив дескрипторов
            TRUE,                       // Ждать ВСЕХ (Wait All)
            INFINITE                    // Ждать бесконечно
        );
    }

    cout << "All Downloader processes terminated." << endl;

    // 7. Корректное закрытие дескрипторов
    for (HANDLE h : downloader_handles) {
        CloseHandle(h);
    }
    CloseHandle(hSemaphore);
    CloseHandle(hMutex);
    CloseHandle(hEvent);

    cout << "\nBrowser.exe finished gracefully." << endl;
    return 0;
}
