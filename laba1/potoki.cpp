#include <iostream>
#include <vector>
#include <cstdlib>
#include <windows.h>
#include <process.h>

using namespace std;

struct ThreadData
{
    vector<int> array;
};

unsigned __stdcall workerThread(void* pData)
{
    ThreadData* data = (ThreadData*)pData;

    cout << " Элементы, кратные 3: ";
    bool found = false;

    for (int num : data->array)
    {
        if (num != 0 && num % 3 == 0)
        {
            cout << num << " ";
            found = true;
        }
     
        Sleep(100);
    }

    if (!found)
    {
        cout << "няма элементов :(";
    }
    cout << endl;

    return 0;
}

int main()
{
    setlocale(LC_ALL, "RUS");
    int size;
    cout << "Введите размер массива: ";
    cin >> size;

    
    vector<int> arr(size);
    char choice;
    cout << "Заполнить массив случайно? (1-ес): ";
    cin >> choice;

    if (choice == '1')
    {
        for (int i = 0; i < size; ++i) 
        {
            arr[i] = rand() % 101 - 50;
        }
        cout << " массив: ";
        for (int num : arr) 
        {
            cout << num << " ";
        }
        cout << endl;
    }
    else {
        cout << "Введите " << size << " элементов массива:" << endl;
        for (int i = 0; i < size; ++i) {
            cin >> arr[i];
        }
    }


    int suspendTimeMs;
    cout << "время приостановки потока worker : ";
    cin >> suspendTimeMs;

    ThreadData threadData;
    threadData.array = arr;

  
    HANDLE hThread;
    unsigned threadID;

    
    hThread = (HANDLE)_beginthreadex(NULL, 0, workerThread, &threadData, 0, &threadID);

    if (hThread == NULL) 
    {
        cerr << "Ошибка создания потока!!!!" << endl;
        return 1;
    }

    cout << " Идентификатор потока: " << threadID << endl;
    cout << " Дескриптор потока: " << hThread << endl;

   
    cout << "Приостановка потока worker..." << endl;
    DWORD suspendResult = SuspendThread(hThread);
    if (suspendResult == (DWORD)-1) 
    {
        cerr << "Ошибка приостановки потока!" << endl;
        CloseHandle(hThread);
        return 1;
    }

    cout << suspendTimeMs << " мс перед запуском..." << endl;
    Sleep(suspendTimeMs);


    cout << "Запуск потока worker...пам" << endl;
    DWORD resumeResult = ResumeThread(hThread);
    if (resumeResult == (DWORD)-1)
    {
        cerr << "Ошибка запуска потока!!!!!!!!!!!" << endl;
        CloseHandle(hThread);
        return 1;
    }

    cout << "Ожидание завершения потока worker..." << endl;
    DWORD waitResult = WaitForSingleObject(hThread, INFINITE);

    if (waitResult == WAIT_OBJECT_0) 
    {
        cout << "все получилось ура ура !!!!!" << endl;
    }
    else 
    {
        cerr << "ЕРРОР" << endl;
    }

    CloseHandle(hThread);
 

    return 0;
}