#include <iostream>
#include <windows.h>
#include <vector>
#include <process.h>

using namespace std;

// ��������� ��� �������� ������ � ����� marker
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

// ������� ������ marker
unsigned __stdcall markerThread(void* param) {
    MarkerData* data = (MarkerData*)param;

    // ������� ������� ������ ������
    WaitForSingleObject(data->startEvent, INFINITE);

    srand(data->markerId); // �������������� ��������� ��������� �����

    int markedCount = 0;
    int lastIndex = -1;

    while (true) {
        int index = rand() % data->arraySize;

        // ������ � ����������� ������
        EnterCriticalSection(data->cs);

        if (data->array[index] == 0) {
            // ������ �������� - �������� �
            Sleep(5);
            data->array[index] = data->markerId;
            Sleep(5);
            markedCount++;
            lastIndex = index;

            // ������� �� ����������� ������
            LeaveCriticalSection(data->cs);
        }
        else {
            // ������ ������ - ������� �� �����
            LeaveCriticalSection(data->cs);
            break;
        }
    }

    // �������� � ������������� ���������� ������
    cout << "����� " << data->markerId << " �����������. �������� �����: "
        << markedCount << ". ��������� ������: " << lastIndex << endl;

    SetEvent(data->threadFinishedEvent);

    // ���� ������� �� main
    HANDLE events[2] = { data->continueEvent, data->stopEvent };
    DWORD result = WaitForMultipleObjects(2, events, FALSE, INFINITE);

    if (result == WAIT_OBJECT_0 + 1) {
        // �������� ������ �� ���������� - ������� ���� �������
        EnterCriticalSection(data->cs);

        for (int i = 0; i < data->arraySize; i++) {
            if (data->array[i] == data->markerId) {
                data->array[i] = 0;
            }
        }

        LeaveCriticalSection(data->cs);
    }

    // ��������� �����
    _endthreadex(0);
    return 0;
}

// ������� ��� ������ �������
void printArray(int* array, int size) {
    cout << "������� ��������� �������: ";
    for (int i = 0; i < size; i++) {
        cout << array[i] << " ";
    }
    cout << endl;
}

int main() {
    setlocale(LC_ALL, "Russian");

    // 1. ������� � �������������� ������
    int arraySize;
    cout << "������� ������ �������: ";
    cin >> arraySize;

    int* array = new int[arraySize];
    for (int i = 0; i < arraySize; i++) {
        array[i] = 0;
    }

    // 2. ����������� ���������� �������
    int threadCount;
    cout << "������� ���������� ������� marker: ";
    cin >> threadCount;

    // 3. ������� ������� �������������
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

    // 4. ��������� ������ marker
    for (int i = 0; i < threadCount; i++) {
        threadData[i] = new MarkerData{
            i + 1,           // markerId (�������� � 1)
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

    // 5. ���� ����� ������ �� ������ ������
    SetEvent(startEvent);

    // 6. ������� ����������� ����
    int activeThreads = threadCount;

    while (activeThreads > 0) {
        // 6a. ����, ���� ��� �������� ������ ������� � ������������� ���������� ������
        WaitForMultipleObjects(activeThreads, threadFinishedEvents.data(), TRUE, INFINITE);

        // 6b. ������� ������� ��������� �������
        printArray(array, arraySize);

        // 6c. ����������� ����� ������ ��� ����������
        int threadToStop;
        cout << "������� ����� ������ ��� ���������� (1-" << threadCount << "): ";
        cin >> threadToStop;

        if (threadToStop < 1 || threadToStop > threadCount) {
            cout << "�������� ����� ������!" << endl;
            continue;
        }

        int threadIndex = threadToStop - 1;

        // 6d. ������ ������ �� ���������� ���������� ������
        SetEvent(stopEvents[threadIndex]);

        // 6e. ���� ���������� ������
        WaitForSingleObject(threads[threadIndex], INFINITE);

        // 6f. ������� ��������� ������� ����� �������
        cout << "����� ���������� ������ " << threadToStop << ":" << endl;
        printArray(array, arraySize);

        // ������� ����������� ����� �� ��������
        threadFinishedEvents.erase(threadFinishedEvents.begin() + threadIndex);
        activeThreads--;

        // 6g. ������ ������ �� ����������� ������ ���������� �������
        for (int i = 0; i < threadCount; i++) {
            if (i != threadIndex) {
                ResetEvent(threadFinishedEvents[i]);
                SetEvent(continueEvents[i]);
            }
        }
    }

    // ����������� �������
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

    cout << "��� ������ ���������. ��������� ���������." << endl;

    return 0;
}