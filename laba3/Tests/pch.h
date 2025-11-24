#pragma once

// Отключаем предупреждения для Google Test
#define _SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING
#define GTEST_HAS_PTHREAD 0

// Для поддержки кириллицы в Windows
#ifdef _WIN32
#include <windows.h>
#endif

// Игнорируем предупреждения инициализации в Google Test
#pragma warning(push)
#pragma warning(disable:26495)
#include "gtest/gtest.h"
#pragma warning(pop)