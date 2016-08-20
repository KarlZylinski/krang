#pragma once

struct Allocator;

enum struct LexTokenType
{
    Name,
    ArgStart,
    ArgEnd,
    ScopeStart,
    ScopeEnd,
    Int32Literal,
    StatementEnd,
    EndOfFile,
    Assignment,
    Operator
};

struct LexToken 
{
    LexTokenType type;
    char* val;
    unsigned len;
};

struct LexerResult
{
    LexToken* data;
    size_t num;
    Allocator* allocator;
};

LexerResult lex(char* data, size_t size, Allocator* allocator);
