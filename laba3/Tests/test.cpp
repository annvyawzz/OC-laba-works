#include "pch.h"
#include <gtest/gtest.h>
#include "MarkerManager.h"
#include <thread>
#include <chrono>

// Базовые тесты
TEST(MarkerTest, InitializationTest) {
    MarkerManager manager;
    EXPECT_TRUE(manager.Initialize(10, 2));
    EXPECT_EQ(manager.GetArraySize(), 10);
    manager.Cleanup();
}

TEST(MarkerTest, ArrayInitiallyZero) {
    MarkerManager manager;
    manager.Initialize(5, 1);

    int* array = manager.GetSharedArray();
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(array[i], 0);
    }

    manager.Cleanup();
}

TEST(MarkerTest, CleanupAfterTermination) {
    MarkerManager manager;
    EXPECT_TRUE(manager.Initialize(10, 1));

    // Сразу завершаем поток
    manager.TerminateThread(0);

    // Проверяем, что все элементы очищены
    int* array = manager.GetSharedArray();
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ(array[i], 0);
    }

    manager.Cleanup();
}

// Упрощенные тесты без GetContinueEvents
TEST(MarkerTest, SingleThreadBasic) {
    MarkerManager manager;
    EXPECT_TRUE(manager.Initialize(5, 1));

    // Простая проверка инициализации
    EXPECT_NE(manager.GetSharedArray(), nullptr);

    manager.Cleanup();
}

TEST(MarkerTest, MultiThreadBasic) {
    MarkerManager manager;
    EXPECT_TRUE(manager.Initialize(10, 3));

    // Проверяем что массив создан
    int* array = manager.GetSharedArray();
    EXPECT_NE(array, nullptr);

    // Проверяем размер
    EXPECT_EQ(manager.GetArraySize(), 10);

    manager.Cleanup();
}

TEST(MarkerTest, ActiveMarkersTest) {
    MarkerManager manager;
    manager.Initialize(8, 2);

    // Проверяем что активные маркеры инициализированы правильно
    auto activeMarkers = manager.GetActiveMarkers();
    EXPECT_EQ(activeMarkers.size(), 2);
    EXPECT_TRUE(activeMarkers[0]);
    EXPECT_TRUE(activeMarkers[1]);

    manager.Cleanup();
}

TEST(MarkerTest, CleanupTest) {
    MarkerManager manager;
    manager.Initialize(10, 1);
    manager.Cleanup();
    SUCCEED();
}