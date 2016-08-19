#pragma once
#include <assert.h>
#define Assert(cond, msg) assert(cond && msg)
#define Error(msg) assert(false && msg)