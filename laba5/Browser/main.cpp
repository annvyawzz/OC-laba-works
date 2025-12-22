#include <iostream>
#include <string>
#include <vector>    
#include <windows.h> 
#include "Protocol.h"

int main() {
    int N, M;
    std::cout << "Enter number of Workers (N): "; std::cin >> N;
    std::cout << "Enter number of Tasks (M): ";   std::cin >> M;

    std::vector<HANDLE> hInPipes(N), hOutPipes(N), hProcesses(N);

    for (int i = 0; i < N; i++) {
        std::string inName = "\\\\.\\pipe\\worker_in_" + std::to_string(i);
        std::string outName = "\\\\.\\pipe\\worker_out_" + std::to_string(i);

        hInPipes[i] = CreateNamedPipeA(inName.c_str(), PIPE_ACCESS_OUTBOUND, PIPE_TYPE_MESSAGE | PIPE_WAIT, 1, 0, 0, 0, NULL);
        hOutPipes[i] = CreateNamedPipeA(outName.c_str(), PIPE_ACCESS_INBOUND, PIPE_TYPE_MESSAGE | PIPE_WAIT, 1, 0, 0, 0, NULL);

        STARTUPINFOA si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        std::string cmd = "Worker.exe " + std::to_string(i);
        CreateProcessA(NULL, (LPSTR)cmd.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
        hProcesses[i] = pi.hProcess;

        ConnectNamedPipe(hInPipes[i], NULL);
        ConnectNamedPipe(hOutPipes[i], NULL);
    }

    for (int j = 0; j < M; j++) {
        int workerIdx = j % N;
        Task t = { TASK_XOR, 5, 0xF, "Hello" }; // Пример данных

        DWORD written, read;
        WriteFile(hInPipes[workerIdx], &t, sizeof(Task), &written, NULL);

        Result r;
        ReadFile(hOutPipes[workerIdx], &r, sizeof(Result), &read, NULL);
        // std::cout << "Task " << j << " processed by Worker " << workerIdx << ". Result: " << r.data << std::endl;
        std::cout << "Task " << j << " processed by Worker " << workerIdx << ". Result: ";
        std::cout.write(r.data, r.dataSize);
        std::cout << std::endl;
    }

    for (int i = 0; i < N; i++) 
    {
        Task exitTask = { TASK_EXIT, 0, 0, "" };
        DWORD written;
        WriteFile(hInPipes[i], &exitTask, sizeof(Task), &written, NULL);

        WaitForSingleObject(hProcesses[i], INFINITE);
        CloseHandle(hInPipes[i]);
        CloseHandle(hOutPipes[i]);
        CloseHandle(hProcesses[i]);
    }

    std::cout << "All workers finished. Exiting." << std::endl;
    return 0;
}
