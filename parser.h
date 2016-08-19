#pragma once
#include <stdio.h>
#include "dynamic_array.h"

struct Allocator;
struct LexToken;
struct ParseNode;

struct ParseScope
{
    DynamicArray<ParseNode> nodes;
};

ParseScope parse(Allocator* alloc, const LexToken* lex_tokens, size_t num_lex_tokens);
