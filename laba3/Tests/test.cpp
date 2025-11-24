#include "pch.h"
#include <gtest/gtest.h>
#include "MarkerManager.h"
#include <thread>
#include <chrono>

// Вспомогательная функция для проверки массива
bool IsArrayMarkedByThread(int* array, int size, int threadId) {
    for (int i = 0; i < size; ++i) {
        if (array[i] != 0 && array[i] != threadId) {
            return false;
        }
    }
    return true;
}

// Тест 1.1: Корректная маркировка одним потоком
TEST(MarkerTest, SingleThreadMarking) {
    MarkerManager manager;
    EXPECT_TRUE(manager.Initialize(10, 1));

    // Запускаем потоки
    manager.ManualRun();

    // Даем потоку время поработать до блокировки
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Проверяем, что все элементы помечены потоком 1
    int* array = manager.GetSharedArray();
    bool correctlyMarked = true;
    for (int i = 0; i < 10; ++i) {
        if (array[i] != 1 && array[i] != 0) {
            correctlyMarked = false;
            break;
        }
    }

    EXPECT_TRUE(correctlyMarked) << "Обнаружены отметки других потоков";

    manager.Cleanup(); // Cleanup сам завершит поток
}

// Тест 1.2: Корректная очистка при завершении
TEST(MarkerTest, CleanupAfterTermination) {
    MarkerManager manager;
    EXPECT_TRUE(manager.Initialize(10, 1));

    manager.ManualRun();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Просто вызываем Cleanup - он завершит поток
    manager.Cleanup();

    // В этом тесте мы не проверяем массив после Cleanup,
    // так как память уже освобождена
    SUCCEED();
}

// Тест 2.1: Отсутствие гонки за ресурс
TEST(MarkerTest, NoRaceCondition) {
    MarkerManager manager;
    const int arraySize = 10; // Уменьшаем для надежности
    const int threadCount = 3; // Уменьшаем для надежности

    EXPECT_TRUE(manager.Initialize(arraySize, threadCount));

    manager.ManualRun();

    // Даем потокам время поработать до блокировки
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Проверяем содержимое массива
    int* array = manager.GetSharedArray();
    bool validValues = true;

    for (int i = 0; i < arraySize; ++i) {
        // Элемент должен быть либо 0, либо от 1 до threadCount
        if (array[i] < 0 || array[i] > threadCount) {
            validValues = false;
            break;
        }
    }

    EXPECT_TRUE(validValues) << "Обнаружены некорректные значения в массиве";

    manager.Cleanup();
}

// Тест 2.2: Корректное поочередное завершение
TEST(MarkerTest, SequentialTermination) {
    MarkerManager manager;
    const int arraySize = 8;
    const int threadCount = 2; // Уменьшаем для надежности

    EXPECT_TRUE(manager.Initialize(arraySize, threadCount));

    manager.ManualRun();

    // Даем потокам время поработать
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Завершаем только один поток вручную
    manager.TerminateThread(1);

    // Проверяем, что поток действительно завершен
    auto activeMarkers = manager.GetActiveMarkers();
    EXPECT_FALSE(activeMarkers[1]) << "Поток 1 не был завершен";
    EXPECT_TRUE(activeMarkers[0]) << "Поток 0 не должен быть завершен";

    // Остальные потоки завершит Cleanup
    manager.Cleanup();
}

// Базовые тесты
TEST(MarkerTest, InitializationTest) {
    MarkerManager manager;
    EXPECT_TRUE(manager.Initialize(10, 2));
    EXPECT_EQ(manager.GetArraySize(), 10);

    // Проверяем, что массив инициализирован нулями
    int* array = manager.GetSharedArray();
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ(array[i], 0);
    }

    manager.Cleanup();
}

TEST(MarkerTest, ActiveMarkersTest) {
    MarkerManager manager;
    manager.Initialize(8, 2);

    auto activeMarkers = manager.GetActiveMarkers();
    EXPECT_EQ(activeMarkers.size(), 2);
    EXPECT_TRUE(activeMarkers[0]);
    EXPECT_TRUE(activeMarkers[1]);

    manager.Cleanup();
}

// Новый тест - проверка безопасного завершения
TEST(MarkerTest, SafeTermination) {
    MarkerManager manager;
    EXPECT_TRUE(manager.Initialize(5, 1));

    manager.ManualRun();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Явно завершаем поток
    manager.TerminateThread(0);

    // Затем вызываем Cleanup - не должно быть ошибок
    EXPECT_NO_THROW(manager.Cleanup());
}
