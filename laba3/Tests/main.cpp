#include "pch.h"
#include "MarkerManager.h"
#include <iostream>

int main(int argc, char** argv)
{
    // Всегда запускаем тесты, потом интерактивный режим
    std::cout << "=== ЗАПУСК ТЕСТОВ ===" << std::endl;
    ::testing::InitGoogleTest(&argc, argv);
    int testResult = RUN_ALL_TESTS();

    std::cout << "\n=== ИНТЕРАКТИВНЫЙ РЕЖИМ ===" << std::endl;
    MarkerManager manager;
    int arraySize, markerCount;

    std::cout << "Введите размер массива: ";
    std::cin >> arraySize;
    std::cout << "Введите количество потоков marker: ";
    std::cin >> markerCount;

    if (manager.Initialize(arraySize, markerCount)) {
        manager.Run();
    }
    else {
        std::cout << "Ошибка инициализации!" << std::endl;
    }

    manager.Cleanup();

    std::cout << "Нажмите Enter для выхода...";
    std::cin.ignore();
    std::cin.get();

    return testResult;
}
