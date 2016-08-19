#pragma once

struct GeneratedCode
{
    char* code;
    unsigned len;
};

struct Allocator;
struct ParseScope;

GeneratedCode generate_code(Allocator* alloc, const ParseScope& ps);
