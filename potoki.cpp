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

    cout << " ��������, ������� 3: ";
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
        cout << "���� ��������� :(";
    }
    cout << endl;

    return 0;
}

int main()
{
    setlocale(LC_ALL, "RUS");
    int size;
    cout << "������� ������ �������: ";
    cin >> size;

    
    vector<int> arr(size);
    char choice;
    cout << "��������� ������ ��������? (1-��): ";
    cin >> choice;

    if (choice == '1')
    {
        for (int i = 0; i < size; ++i) 
        {
            arr[i] = rand() % 101 - 50;
        }
        cout << " ������: ";
        for (int num : arr) 
        {
            cout << num << " ";
        }
        cout << endl;
    }
    else {
        cout << "������� " << size << " ��������� �������:" << endl;
        for (int i = 0; i < size; ++i) {
            cin >> arr[i];
        }
    }


    int suspendTimeMs;
    cout << "����� ������������ ������ worker : ";
    cin >> suspendTimeMs;

    ThreadData threadData;
    threadData.array = arr;

  
    HANDLE hThread;
    unsigned threadID;

    
    hThread = (HANDLE)_beginthreadex(NULL, 0, workerThread, &threadData, 0, &threadID);

    if (hThread == NULL) 
    {
        cerr << "������ �������� ������!!!!" << endl;
        return 1;
    }

    cout << " ������������� ������: " << threadID << endl;
    cout << " ���������� ������: " << hThread << endl;

   
    cout << "������������ ������ worker..." << endl;
    DWORD suspendResult = SuspendThread(hThread);
    if (suspendResult == (DWORD)-1) 
    {
        cerr << "������ ������������ ������!" << endl;
        CloseHandle(hThread);
        return 1;
    }

    cout << suspendTimeMs << " �� ����� ��������..." << endl;
    Sleep(suspendTimeMs);


    cout << "������ ������ worker...���" << endl;
    DWORD resumeResult = ResumeThread(hThread);
    if (resumeResult == (DWORD)-1)
    {
        cerr << "������ ������� ������!!!!!!!!!!!" << endl;
        CloseHandle(hThread);
        return 1;
    }

    cout << "�������� ���������� ������ worker..." << endl;
    DWORD waitResult = WaitForSingleObject(hThread, INFINITE);

    if (waitResult == WAIT_OBJECT_0) 
    {
        cout << "��� ���������� ��� ��� !!!!!" << endl;
    }
    else 
    {
        cerr << "�����" << endl;
    }

    CloseHandle(hThread);
 

    return 0;
}