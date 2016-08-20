#pragma once

struct Allocator;

struct Token
{
    enum struct Type
    {
        Name,
        ArgStart,
        ArgEnd,
        ScopeStart,
        ScopeEnd,
        Literal,
        StatementEnd,
        EndOfFile,
        Assignment,
        Operator,
        Arrow
    };

    Type type;
    char* val;
    unsigned len;
};

struct TokenizerResult
{
    Token* data;
    size_t num;
    Allocator* allocator;
};

TokenizerResult tokenize(char* data, size_t size, Allocator* allocator);
