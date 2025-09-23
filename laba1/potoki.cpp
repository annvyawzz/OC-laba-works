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

    cout << " Ýëåìåíòû, êðàòíûå 3: ";
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
        cout << "íÿìà ýëåìåíòîâ :(";
    }
    cout << endl;

    return 0;
}

int main()
{
    setlocale(LC_ALL, "RUS");
    int size;
    cout << "Ââåäèòå ðàçìåð ìàññèâà: ";
    cin >> size;

    
    vector<int> arr(size);
    char choice;
    cout << "Çàïîëíèòü ìàññèâ ñëó÷àéíî? (1-åñ): ";
    cin >> choice;

    if (choice == '1')
    {
        for (int i = 0; i < size; ++i) 
        {
            arr[i] = rand() % 101 - 50;
        }
        cout << " ìàññèâ: ";
        for (int num : arr) 
        {
            cout << num << " ";
        }
        cout << endl;
    }
    else {
        cout << "Ââåäèòå " << size << " ýëåìåíòîâ ìàññèâà:" << endl;
        for (int i = 0; i < size; ++i) {
            cin >> arr[i];
        }
    }


    int suspendTimeMs;
    cout << "âðåìÿ ïðèîñòàíîâêè ïîòîêà worker : ";
    cin >> suspendTimeMs;

    ThreadData threadData;
    threadData.array = arr;

  
    HANDLE hThread;
    unsigned threadID;

    
    hThread = (HANDLE)_beginthreadex(NULL, 0, workerThread, &threadData, CREATE_SUSPENDED, &threadID);

    if (hThread == NULL) 
    {
        cerr << "Îøèáêà ñîçäàíèÿ ïîòîêà!!!!" << endl;
        return 1;
    }

    cout << " Èäåíòèôèêàòîð ïîòîêà: " << threadID << endl;
    cout << " Äåñêðèïòîð ïîòîêà: " << hThread << endl;

   
    cout << "Ïðèîñòàíîâêà ïîòîêà worker..." << endl;
    DWORD suspendResult = SuspendThread(hThread);
    if (suspendResult == (DWORD)-1) 
    {
        cerr << "Îøèáêà ïðèîñòàíîâêè ïîòîêà!" << endl;
        CloseHandle(hThread);
        return 1;
    }

    cout << suspendTimeMs << " ìñ ïåðåä çàïóñêîì..." << endl;
    Sleep(suspendTimeMs);


    cout << "Çàïóñê ïîòîêà worker...ïàì" << endl;
    DWORD resumeResult = ResumeThread(hThread);
    if (resumeResult == (DWORD)-1)
    {
        cerr << "Îøèáêà çàïóñêà ïîòîêà!!!!!!!!!!!" << endl;
        CloseHandle(hThread);
        return 1;
    }

    cout << "Îæèäàíèå çàâåðøåíèÿ ïîòîêà worker..." << endl;
    DWORD waitResult = WaitForSingleObject(hThread, INFINITE);

    if (waitResult == WAIT_OBJECT_0) 
    {
        cout << "âñå ïîëó÷èëîñü óðà óðà !!!!!" << endl;
    }
    else 
    {
        cerr << "ÅÐÐÎÐ" << endl;
    }

    CloseHandle(hThread);
 

    return 0;

}
