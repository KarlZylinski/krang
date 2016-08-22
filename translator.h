#pragma once
#include "dynamic_array.h"

struct AsmTranslationResult
{
    char* data;
    size_t len;
};

struct Allocator;
struct AsmChunk;

AsmTranslationResult translate_to_asm(Allocator* allocator, const DynamicArray<AsmChunk>& chunks);