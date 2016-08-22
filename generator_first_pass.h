#pragma once
#include "dynamic_array.h"

struct AsmChunk;
struct ParseScope;
struct Allocator;

struct GeneratedCodeFirstPass
{
    DynamicArray<AsmChunk> chunks;
};

GeneratedCodeFirstPass generate_first_pass(Allocator* allocator, const ParseScope& ps);