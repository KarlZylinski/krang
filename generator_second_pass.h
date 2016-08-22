#pragma once
#include "dynamic_array.h"

struct AsmChunk;
struct ParseScope;
struct Allocator;

struct GeneratedCodeSecondPass
{
    DynamicArray<AsmChunk> chunks;
};

struct GeneratedCodeFirstPass;

GeneratedCodeSecondPass generate_second_pass(Allocator* allocator, const DynamicArray<AsmChunk>& first_pass);
