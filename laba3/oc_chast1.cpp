#include <iostream>
#include <windows.h>
#include <vector>
#include <process.h>

using namespace std;

struct MarkerData
{
    int markerId;
    int arraySize;
    int* array;
    CRITICAL_SECTION* cs;
    HANDLE startEvent;
    HANDLE stopEvent;
    HANDLE continueEvent;
    HANDLE threadFinishedEvent;
};

unsigned __stdcall markerThread(void* param)
{
    MarkerData* data = (MarkerData*)param;

    WaitForSingleObject(data->startEvent, INFINITE);

    srand(data->markerId); 

    int markedCount = 0;
    int lastIndex = -1;

    while (true) 
    {
        int index = rand() % data->arraySize;
\
        EnterCriticalSection(data->cs);

        if (data->array[index] == 0)
        {
            Sleep(5);
            data->array[index] = data->markerId;
            Sleep(5);
            markedCount++;
            lastIndex = index;

            LeaveCriticalSection(data->cs);
        }
        else {
            LeaveCriticalSection(data->cs);
            break;
        }
    }

    cout << "Ïîòîê " << data->markerId << " îñòàíîâèëñÿ. Ïîìå÷åíî ÿ÷ååê: "
        << markedCount << ". Ïîñëåäíÿÿ ÿ÷åéêà: " << lastIndex << endl;

    SetEvent(data->threadFinishedEvent);

    HANDLE events[2] = { data->continueEvent, data->stopEvent };
    DWORD result = WaitForMultipleObjects(2, events, FALSE, INFINITE);

    if (result == WAIT_OBJECT_0 + 1) 
    {
        EnterCriticalSection(data->cs);

        for (int i = 0; i < data->arraySize; i++) {
            if (data->array[i] == data->markerId) {
                data->array[i] = 0;
            }
        }

        LeaveCriticalSection(data->cs);
    }
    _endthreadex(0);
    return 0;
}

void printArray(int* array, int size) {
    cout << "Òåêóùåå ñîñòîÿíèå ìàññèâà: ";
    for (int i = 0; i < size; i++) {
        cout << array[i] << " ";
    }
    cout << endl;
}

int main() 
{
    setlocale(LC_ALL, "Rus");

    int arraySize;
    cout << "Ââåäèòå ðàçìåð ìàññèâà: ";
    cin >> arraySize;

    int* array = new int[arraySize];
    for (int i = 0; i < arraySize; i++) {
        array[i] = 0;
    }

    // 2. Çàïðàøèâàåì êîëè÷åñòâî ïîòîêîâ
    int threadCount;
    cout << "Ââåäèòå êîëè÷åñòâî ïîòîêîâ marker: ";
    cin >> threadCount;

    // 3. Ñîçäàåì îáúåêòû ñèíõðîíèçàöèè
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

    // 4. Çàïóñêàåì ïîòîêè marker
    for (int i = 0; i < threadCount; i++)
        {
        threadData[i] = new MarkerData{
            i + 1,           // markerId (íà÷èíàåì ñ 1)
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

    // 5. Äàåì îáùèé ñèãíàë íà íà÷àëî ðàáîòû
    SetEvent(startEvent);

    // 6. Ãëàâíûé óïðàâëÿþùèé öèêë
    int activeThreads = threadCount;

    while (activeThreads > 0) {
        // 6a. Æäåì, ïîêà âñå àêòèâíûå ïîòîêè ñîîáùàò î íåâîçìîæíîñòè ïðîäîëæàòü ðàáîòó
        WaitForMultipleObjects(activeThreads, threadFinishedEvents.data(), TRUE, INFINITE);

        // 6b. Âûâîäèì òåêóùåå ñîñòîÿíèå ìàññèâà
        printArray(array, arraySize);

        // 6c. Çàïðàøèâàåì íîìåð ïîòîêà äëÿ çàâåðøåíèÿ
        int threadToStop;
        cout << "Ââåäèòå íîìåð ïîòîêà äëÿ çàâåðøåíèÿ (1-" << threadCount << "): ";
        cin >> threadToStop;

        if (threadToStop < 1 || threadToStop > threadCount) {
            cout << "Íåâåðíûé íîìåð ïîòîêà!" << endl;
            continue;
        }

        int threadIndex = threadToStop - 1;

        // 6d. Ïîäàåì ñèãíàë íà çàâåðøåíèå âûáðàííîìó ïîòîêó
        SetEvent(stopEvents[threadIndex]);

        // 6e. Æäåì çàâåðøåíèÿ ïîòîêà
        WaitForSingleObject(threads[threadIndex], INFINITE);

        // 6f. Âûâîäèì ñîñòîÿíèå ìàññèâà ïîñëå î÷èñòêè
        cout << "Ïîñëå çàâåðøåíèÿ ïîòîêà " << threadToStop << ":" << endl;
        printArray(array, arraySize);

        // Óáèðàåì çàâåðøåííûé ïîòîê èç àêòèâíûõ
        threadFinishedEvents.erase(threadFinishedEvents.begin() + threadIndex);
        activeThreads--;

        // 6g. Ïîäàåì ñèãíàë íà ïðîäîëæåíèå ðàáîòû îñòàâøèìñÿ ïîòîêàì
        for (int i = 0; i < threadCount; i++) {
            if (i != threadIndex) {
                ResetEvent(threadFinishedEvents[i]);
                SetEvent(continueEvents[i]);
            }
        }
    }

    DeleteCriticalSection(&cs);
    CloseHandle(startEvent);

    for (int i = 0; i < threadCount; i++) 
    {
        CloseHandle(threads[i]);
        CloseHandle(continueEvents[i]);
        CloseHandle(stopEvents[i]);
        CloseHandle(threadFinishedEvents[i]);
        delete threadData[i];
    }

    delete[] array;

    cout << "Âñå ïîòîêè çàâåðøåíû. Ïðîãðàììà çàâåðøåíà." << endl;

    return 0;

}

