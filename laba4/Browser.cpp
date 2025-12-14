#include <iostream>
#include <vector>
#include <string>
#include <windows.h>
#include <ctime>

using namespace std;
const char* SEMAPHORE_NAME = "DownloadSlots_NEW_2024";
const char* MUTEX_NAME = "LogAccessMutex_NEW_2024";
const char* EVENT_NAME = "BrowserClosingEvent_NEW_2024";

int main()
{
    setlocale(LC_ALL, "rus");
    int N_slots = 0;
    int M_downloads = 0;

    cout << "--- Инициализация Browser.exe ---" << endl;

    do {
        cout << "Введите максимальное число одновременных загрузок N: ";
        cin >> N_slots;
        if (N_slots <= 0 || N_slots > 15) {
            cout << "Ошибка: N должно быть от 1 до 15" << endl;
            cin.clear();
            cin.ignore(10000, '\n');
        }
    } while (N_slots <= 0 || N_slots > 10);

    do {
        cout << "Введите общее число файлов для загрузки (M, должно быть > N ): ";
        cin >> M_downloads;
        if (M_downloads <= N_slots) {
            cout << "Ошибка: M должно быть больше, чем N." << endl;
            cin.clear();
            cin.ignore(10000, '\n');
        }
        else if (M_downloads > 30) {
            cout << "Ошибка: M не может быть больше 30" << endl;
            cin.clear();
            cin.ignore(10000, '\n');
        }
    } while (M_downloads <= N_slots || M_downloads > 20);

    cout << "\n2. Создание объектов синхронизации..." << endl;

    // 1. Создаём семафор
    HANDLE hSemaphore = CreateSemaphoreA(
        NULL,
        N_slots,
        N_slots,
        SEMAPHORE_NAME
    );
    if (hSemaphore == NULL) {
        cerr << "Ошибка создания Семафора: " << GetLastError() << endl;
        return 1;
    }
    cout << "Семафор '" << SEMAPHORE_NAME << "' создан с N=" << N_slots << "." << endl;

    // 2. Создаём мьютекс
    HANDLE hMutex = CreateMutexA(
        NULL,
        FALSE,
        MUTEX_NAME
    );
    if (hMutex == NULL) {
        cerr << "Ошибка создания Мьютекса: " << GetLastError() << endl;
        CloseHandle(hSemaphore);
        return 1;
    }
    cout << "Мьютекс '" << MUTEX_NAME << "' создан." << endl;

    // 3. Сначала закрываем старое событие, если оно существует
    HANDLE hOldEvent = OpenEventA(EVENT_ALL_ACCESS, FALSE, EVENT_NAME);
    if (hOldEvent != NULL) {
        cout << "Обнаружено старое событие. Закрываю..." << endl;
        CloseHandle(hOldEvent);
        Sleep(100);
    }

    // 4. Создаём новое событие
    HANDLE hEvent = CreateEventA(
        NULL,
        TRUE,
        FALSE,
        EVENT_NAME
    );
    if (hEvent == NULL) {
        cerr << "Ошибка создания События: " << GetLastError() << endl;
        CloseHandle(hSemaphore);
        CloseHandle(hMutex);
        return 1;
    }
    //cout << "Событие '" << EVENT_NAME << "' создано (Ручной сброс, Изначально: Несигнальное)." << endl;

    // 5. Двойная проверка состояния события
    DWORD state = WaitForSingleObject(hEvent, 0);
    if (state == WAIT_OBJECT_0) {
        cout << "ОШИБКА: Событие в сигнальном состоянии!" << endl;
        ResetEvent(hEvent);

        state = WaitForSingleObject(hEvent, 0);
        if (state == WAIT_OBJECT_0) {
            cout << "Не удалось исправить" << endl;
            CloseHandle(hEvent);

            ULONGLONG tickCount = GetTickCount64();
            string uniqueEventName = string(EVENT_NAME) + "_" + to_string(tickCount);
            hEvent = CreateEventA(NULL, TRUE, FALSE, uniqueEventName.c_str());
            if (hEvent == NULL) {
                cerr << "Критическая ошибка создания события. Выход." << endl;
                CloseHandle(hSemaphore);
                CloseHandle(hMutex);
                return 1;
            }
            //cout << "Создано событие с уникальным именем: " << uniqueEventName << endl;
        }
        else {
          //cout << "Событие исправлено. Теперь в несигнальном состоянии." << endl;
        }
    }
    else {
       // cout << "Событие создано в НЕСИГНАЛЬНОМ состоянии" << endl;
    }

    // 6. Запуск M экземпляров Downloader.exe
    vector<HANDLE> downloader_handles;
    cout << "\n3. Запуск " << M_downloads << " экземпляров Downloader.exe..." << endl;

    for (int i = 0; i < M_downloads; ++i) {
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        string fileName = "file_" + to_string(i + 1) + ".txt";
        string commandLine = "Downloader.exe " + fileName + " " + to_string(i + 1);

        if (!CreateProcessA(
            NULL,
            const_cast<LPSTR>(commandLine.c_str()),
            NULL,
            NULL,
            FALSE,
            0,
            NULL,
            NULL,
            &si,
            &pi
        )) {
            cerr << "Ошибка запуска Downloader #" << i + 1 << ": " << GetLastError() << endl;
            continue;
        }

        downloader_handles.push_back(pi.hProcess);
        CloseHandle(pi.hThread);

        Sleep(50);
    }

    cout << M_downloads << " экземпляров запущено." << endl;

    //cout << "\n--- Browser запущен. Нажмите **Enter**, чтобы закрыть... ---" << endl;
    cout << "Первые " << N_slots << " процессов начнут загрузку сразу..." << endl;
    cout << "Остальные будут ждать освобождения слотов" << endl;
    cout << "Текущий статус: " << downloader_handles.size() << " процессов запущено" << endl;

    cin.ignore();
    cin.get();

    cout << "\n5a. Browser закрывается. Отправка сигнала завершения всем загрузкам..." << endl;

    if (!SetEvent(hEvent)) {
        cerr << "Ошибка установки BrowserClosingEvent: " << GetLastError() << endl;
    }
    else {
        cout << "Сигнал завершения **ОТПРАВЛЕН**" << endl;
    }

    cout << "6. Ожидание завершения всех процессов Downloader (" << downloader_handles.size() << " процессов)..." << endl;

    if (!downloader_handles.empty()) {
        DWORD waitResult = WaitForMultipleObjects(
            static_cast<DWORD>(downloader_handles.size()),
            downloader_handles.data(),
            TRUE,
            10000
        );

        if (waitResult == WAIT_TIMEOUT) {
            cout << "Таймаут! Не все процессы завершились за отведенное время." << endl;
        }
        else if (waitResult == WAIT_FAILED) {
            cerr << "Ошибка WaitForMultipleObjects: " << GetLastError() << endl;
        }
        else {
            cout << "Все процессы Downloader завершены" << endl;
        }
    }
    else {
        cout << "Нет запущенных процессов Downloader" << endl;
    }

    for (HANDLE h : downloader_handles) {
        if (h != NULL) {
            CloseHandle(h);
        }
    }

    if (hSemaphore != NULL) CloseHandle(hSemaphore);
    if (hMutex != NULL) CloseHandle(hMutex);
    if (hEvent != NULL) CloseHandle(hEvent);

    cout << "\nBrowser.exe корректно завершил работу." << endl;
    cout << "Нажмите любую клавишу для выхода...";
    cin.get();

    return 0;
}
