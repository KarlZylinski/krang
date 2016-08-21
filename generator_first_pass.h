#pragma once

struct AsmChunk;
struct ParseScope;
struct Allocator;

struct GeneratedCodeFirstPass
{
    AsmChunk* chunks;
    unsigned num_chunks;
};

GeneratedCodeFirstPass generate_first_pass(Allocator* allocator, const ParseScope& ps);