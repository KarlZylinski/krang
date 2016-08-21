#pragma once
#include <assert.h>
#include <string.h>
#define Assert(cond, msg) assert(cond && msg)
#define Error(msg) assert(false && msg)
#define alignof(x) __alignof(x)

inline int max(int i1, int i2)
{
    return i1 > i2 ? i1 : i2;
}

inline bool str_equal(const char* a, const char* b)
{
    return strcmp(a, b) == 0;
}

inline bool str_equal(const char* a, const char* b, unsigned len)
{
    return memcmp(a, b, len) == 0;
}
