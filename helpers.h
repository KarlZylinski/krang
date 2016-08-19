#pragma once
#include <assert.h>
#define Assert(cond, msg) assert(cond && msg)
#define Error(msg) assert(false && msg)

inline int max(int i1, int i2)
{
    return i1 > i2 ? i1 : i2;
}