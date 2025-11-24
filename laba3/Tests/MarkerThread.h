#pragma once
#include <windows.h>
#include <iostream>
#include <vector>

struct MarkerThreadData
{
    int threadId;
    int arraySize;
    int* sharedArray;
    CRITICAL_SECTION* cs;
    CRITICAL_SECTION* outputCs;
    HANDLE startEvent;
    HANDLE suspendEvent;
    HANDLE terminateEvent;
    HANDLE continueEvent;
};

DWORD WINAPI MarkerThread(LPVOID param);
