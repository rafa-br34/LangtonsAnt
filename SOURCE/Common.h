#pragma once
#include <stdint.h>

using CellType = uint8_t;
using SizeType = int64_t;

#define FLATTEN_2D(X, Y, Width) ((Width) * (Y) + (X))
#define XOR_SHIFT32(S) S ^= (S << 13); S ^= (S >> 17); S ^= (S << 5);

#define _MAKE_STRING(Value) #Value
#define MAKE_STRING(Value) _MAKE_STRING(Value)
#define ON_FAIL(Condition, Operation) if (!(Condition)) { Operation; }

#define ASSERTION_MESSAGE(Condition) "Assertion failed " #Condition " on file: " __FILE__ " at line: " MAKE_STRING(__LINE__)

#ifdef NDEBUG
#define DEBUG_BUILD 0
#else
#define DEBUG_BUILD 1
#endif

#if DEBUG_BUILD == 0
#define CONSTEXPR constexpr

#define DEBUG_PRINT(Format, ...)
#define ASSERT(Condition)
#define ASSERT_MSG(Condition, Format, ...)
#else
#define CONSTEXPR

#include <stdio.h>
#define DEBUG_PRINT(Format, ...)           std::printf(Format, ##__VA_ARGS__)
#define ASSERT(Condition)                  ON_FAIL(Condition, std::printf(ASSERTION_MESSAGE(Condition) "\n"))
#define ASSERT_MSG(Condition, Format, ...) ON_FAIL(Condition, std::printf(ASSERTION_MESSAGE(Condition) " " Format, __VA_ARGS__))
#endif