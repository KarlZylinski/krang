#pragma once

struct Allocator;

enum struct LexTokenType
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
