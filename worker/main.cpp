#include <iostream>
#include <string>
#include <vector>   
#include <windows.h> 
#include "Protocol.h"

int main(int argc, char* argv[])
{
    if (argc < 2) return 1;
    std::string id = argv[1];

    std::string inPipeName = "\\\\.\\pipe\\worker_in_" + id;
    std::string outPipeName = "\\\\.\\pipe\\worker_out_" + id;

   
    HANDLE hInPipe = CreateFileA(inPipeName.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    HANDLE hOutPipe = CreateFileA(outPipeName.c_str(), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

    if (hInPipe == INVALID_HANDLE_VALUE || hOutPipe == INVALID_HANDLE_VALUE) return 2;

    Task task;
    DWORD bytesProcessed;

    while (true) 
    {
        // 1. Читаем задачу
        if (!ReadFile(hInPipe, &task, sizeof(Task), &bytesProcessed, NULL)) break;

        // 2. Проверяем на выход
        if (task.type == TASK_EXIT) break;

        // 3. Выполняем XOR
        Result res;
        res.dataSize = task.dataSize;
        for (int i = 0; i < task.dataSize; i++) {
            res.data[i] = task.data[i] ^ task.xorKey;
        }

        // 4. Отправляем результат
        WriteFile(hOutPipe, &res, sizeof(Result), &bytesProcessed, NULL);
    }

    CloseHandle(hInPipe);
    CloseHandle(hOutPipe);
    return 0;
}
