#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>

using namespace std;

const char* CHILD_ARG = "child";

void PrintError(const string& message)
{
    cerr << "Ошибка: " << message << " (код: " << GetLastError() << ")" << endl;
}

//для работы в режиме потомка
void RunChildProcess()
{
    int size;

    DWORD bytesRead;
    if (!ReadFile(GetStdHandle(STD_INPUT_HANDLE), &size, sizeof(size), &bytesRead, NULL)) 
    {
        PrintError("Не удалось прочитать размер массива");
        ExitProcess(1);
    }

    vector<int> array(size);
    for (int i = 0; i < size; ++i)
    {
        if (!ReadFile(GetStdHandle(STD_INPUT_HANDLE), &array[i], sizeof(array[i]), &bytesRead, NULL)) {
            PrintError("Не удалось прочитать элемент массива");
            ExitProcess(1);
        }
    }

    //вариант 6
    int count = 0;
    for (int i = 0; i < size; ++i) 
    {
        if (array[i] % 3 == 0)
        {
            count++;
        }
    }

    
    DWORD bytesWritten;
    if (!WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), &count, sizeof(count), &bytesWritten, NULL))
    {
        PrintError("Не удалось записать результат");
        ExitProcess(1);
    }

    ExitProcess(0);
}

//работа в режиме родителя
void RunParentProcess() 
{
    // Инициализация генератора случайных чисел
    srand(static_cast<unsigned int>(time(nullptr)));

    int size;
    cout << "Введите размер массива: ";
    cin >> size;

    if (size <= 0)
    {
        cerr << "положительным число`д´!" << endl;
        return;
    }

    vector<int> array(size);

    char choice;
    cout << "Хотите ввести элементы вручную? (1): ";
    cin >> choice;

    if (choice == '1') 
    {
        cout << "Введите " << size << " элементов массива:" << endl;
        for (int i = 0; i < size; ++i) 
        {
            cout << "Элемент " << i + 1 << ": ";
            cin >> array[i];
        }
    }
    else
    {
        
        int min, max;
        cout << "Введите мин знач: ";
        cin >> min;
        cout << "Введите макс знач: ";
        cin >> max;

        for (int i = 0; i < size; ++i) 
        {
            array[i] = min + rand() % (max - min + 1);
        }

        cout << "Сгенерированный массив: ";
        for (int i = 0; i < size; ++i) 
        {
            cout << array[i] << " ";
        }
        cout << endl;
    }

    HANDLE hStdoutRead, hStdoutWrite;  // Канал для чтения результата
    HANDLE hStdinRead, hStdinWrite;    // Канал для записи данных

    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    //канал для stdout потомка
    if (!CreatePipe(&hStdoutRead, &hStdoutWrite, &saAttr, 0)) 
    {
        PrintError("Не удалось создать канал stdout");
        return;
    }

    //канал для stdin потомка
    if (!CreatePipe(&hStdinRead, &hStdinWrite, &saAttr, 0))
    {
        PrintError("Не удалось создать канал stdin");
        CloseHandle(hStdoutRead);
        CloseHandle(hStdoutWrite);
        return;
    }

    // Настройка структур для создания процесса - используем STARTUPINFOA
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Настройка перенаправления стандартных потоков
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = hStdinRead;    // stdin-канал чтения
    si.hStdOutput = hStdoutWrite; // stdout-канал записи  
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);

    // Получение пути к текущему исполняемому файлу
    char modulePath[MAX_PATH];
    if (GetModuleFileNameA(NULL, modulePath, MAX_PATH) == 0)
    {
        PrintError("Не удалось получить путь к исполняемому файлу");
        CloseHandle(hStdoutRead);
        CloseHandle(hStdoutWrite);
        CloseHandle(hStdinRead);
        CloseHandle(hStdinWrite);
        return;
    }

    // Формирование командной строки
    string commandLine = string("\"") + modulePath + "\" " + CHILD_ARG;

    //дочерний процесс с CreateProcessA
    if (!CreateProcessA(
        NULL,                   // Имя модуля
        (LPSTR)commandLine.c_str(), // Командная строка
        NULL,                   // Атрибуты безопасности процесса
        NULL,                   // Атрибуты безопасности потока
        TRUE,                   // Наследование дескрипторов
        0,                      // Флаги создания
        NULL,                   // Окружение
        NULL,                   // Текущий каталог
        &si,                    // STARTUPINFOA
        &pi)) {                 // PROCESS_INFORMATION

        PrintError("Не удалось создать дочерний процесс");
        CloseHandle(hStdoutRead);
        CloseHandle(hStdoutWrite);
        CloseHandle(hStdinRead);
        CloseHandle(hStdinWrite);
        return;
    }

    CloseHandle(hStdinRead);   
    CloseHandle(hStdoutWrite); 

    // Запись данных в канал для потомка
    DWORD bytesWritten;

    if (!WriteFile(hStdinWrite, &size, sizeof(size), &bytesWritten, NULL))
    {
        PrintError("Не удалось записать размер массива");
        CloseHandle(hStdinWrite);
        CloseHandle(hStdoutRead);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return;
    }
    for (int i = 0; i < size; ++i)
    {
        if (!WriteFile(hStdinWrite, &array[i], sizeof(array[i]), &bytesWritten, NULL)) {
            PrintError("Не удалось записать элемент массива");
            CloseHandle(hStdinWrite);
            CloseHandle(hStdoutRead);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return;
        }
    }

    // ВАЖНО: Закрываем дескриптор записи (сигнал EOF для потомка)
    CloseHandle(hStdinWrite);

    int result;
    DWORD bytesRead;
    if (!ReadFile(hStdoutRead, &result, sizeof(result), &bytesRead, NULL)) {
        PrintError("Не удалось прочитать результат");
        CloseHandle(hStdoutRead);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    cout << "Результат: количество элементов, кратных 3 = " << result << endl;

    CloseHandle(hStdoutRead);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

int main(int argc, char* argv[])
{
    setlocale(LC_ALL, "rus");
    if (argc > 1 && string(argv[1]) == CHILD_ARG)
    {
        RunChildProcess();
    }
    else
    {
        RunParentProcess();
    }

    return 0;
}
