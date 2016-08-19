#pragma once

enum struct LexTokenType
{
    Name,
    ArgStart,
    ArgEnd,
    ScopeStart,
    ScopeEnd,
    Int32Literal,
    StatementEnd,
    EndOfFile
};

struct LexToken 
{
    LexTokenType type;
    char* val;
    unsigned len;
};

size_t lex(char* data, unsigned size, LexToken* out);
