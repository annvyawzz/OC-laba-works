#include "pch.h"
#include "MarkerManager.h"
#include <iostream>
#include <locale>
using namespace std;

int main(int argc, char** argv)
{
    setlocale(LC_ALL, "rus");

    cout << "=== ЗАПУСК ТЕСТОВ ===" << endl;
    ::testing::InitGoogleTest(&argc, argv);
    int testResult = RUN_ALL_TESTS();

   cout << "\n=== ИНТЕРАКТИВНЫЙ РЕЖИМ ===" << endl;
    MarkerManager manager;
    int arraySize, markerCount;

   cout << "Введите размер массива: ";
    cin >> arraySize;
   cout << "Введите количество потоков marker: ";
   cin >> markerCount;

    if (manager.Initialize(arraySize, markerCount))
    {
        manager.Run();
    }
    else {
       cout << "Ошибка инициализации!" << endl;
    }

    manager.Cleanup();

   cout << "Нажмите Enter для выхода...";
   cin.ignore();
   cin.get();

    return testResult;
}
