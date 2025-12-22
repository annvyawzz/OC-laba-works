#pragma once
#include <windows.h> // Œ¡ﬂ«¿“≈À‹ÕŒ œ≈–¬€Ã

#define MAX_DATA_SIZE 256

enum TaskType {
    TASK_XOR,
    TASK_EXIT
};

struct Task {
    TaskType type;
    int dataSize;
    int xorKey;
    char data[MAX_DATA_SIZE];
};

struct Result {
    int dataSize;
    char data[MAX_DATA_SIZE];
};