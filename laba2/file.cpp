#include <windows.h>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

const char* CHILD_ARG = "child";
void PrintError(const string& message)
{
    cerr << "Ошибка: " << message << " (код: " << GetLastError() << ")" << endl;

  int count = 0;
for (int i = 0; i < size; ++i) 
{
    if (array[i] % 3 == 0) {
        count++;
    }
}
}


void RunParentProcess()
{
    srand(static_cast<unsigned int>(time(nullptr)));

    int size;
    cout << "Введите размер массива: ";
    cin >> size;

    if (size <= 0) {
        cerr << "Размер массива должен быть положительным числом!" << endl;
        return;
    }

    vector<int> array(size);

    char choice;
    cout << "Хотите ввести элементы вручную? (y/n): ";
    cin >> choice;

    if (choice == 'y' || choice == 'Y') 
    {
        cout << "Введите " << size << " элементов массива:" << endl;
        for (int i = 0; i < size; ++i) {
            cout << "Элемент " << i + 1 << ": ";
            cin >> array[i];
        }
    }
    else {
        
        int min, max;
        cout << "Введите минимальное значение: ";
        cin >> min;
        cout << "Введите максимальное значение: ";
        cin >> max;

        for (int i = 0; i < size; ++i) {
            array[i] = min + rand() % (max - min + 1);
        }

        cout << "Сгенерированный массив: ";
        for (int i = 0; i < size; ++i) {
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

    if (!CreatePipe(&hStdoutRead, &hStdoutWrite, &saAttr, 0)) 
    {
        PrintError("Не удалось создать канал stdout");
        return;
    }

    if (!CreatePipe(&hStdinRead, &hStdinWrite, &saAttr, 0))
    {
        PrintError("Не удалось создать канал stdin");
        CloseHandle(hStdoutRead);
        CloseHandle(hStdoutWrite);
        return;
    }

