#include "lexer.h"
#include "memory.h"

struct LexerState
{
    char* start;
    char* head;
    char* end;
    LexToken* out;
    size_t out_num;
};

static void add_lex(LexerState* ls, LexTokenType type, char* val, unsigned len)
{
    LexToken& t = *ls->out;
    t.type = type;
    t.val = val;
    t.len = len;
    ++ls->out;
    ++ls->out_num;
}

static void lex_name(LexerState* ls)
{
    char* val = ls->head;

    while (*ls->head >= 'a' && *ls->head <= 'z' || *ls->head >= '0' && *ls->head <= '9')
        ++ls->head;

    add_lex(ls, LexTokenType::Name, val, mem_ptr_diff(val, ls->head));
}

static void lex_num_literal(LexerState* ls)
{
    char* val = ls->head;

    while (*ls->head >= '0' && *ls->head <= '9')
        ++ls->head;

    add_lex(ls, LexTokenType::Int32Literal, val, mem_ptr_diff(val, ls->head));
}

size_t lex(char* data, unsigned size, LexToken* out)
{
    LexerState ls = {};
    ls.start = data;
    ls.head = ls.start;
    ls.end = data + size;
    ls.out = out;
    ls.out_num = 0;

    while (ls.head <= ls.end)
    {
        const char c = *ls.head;
        char* c_ptr = ls.head;

        if (c >= 'a' && c <= 'z')
        {
            lex_name(&ls);
        }
        else if (c == '(')
        {
            add_lex(&ls, LexTokenType::ArgStart, ls.head, 1);
            ++ls.head;
        }
        else if (c == ')')
        {
            add_lex(&ls, LexTokenType::ArgEnd, ls.head, 1);
            ++ls.head;
        }
        else if (c == '{')
        {
            add_lex(&ls, LexTokenType::ScopeStart, ls.head, 1);
            ++ls.head;
        }
        else if (c == '}')
        {
            add_lex(&ls, LexTokenType::ScopeEnd, ls.head, 1);
            ++ls.head;
        }
        else if (c >= '0' && c <= '9')
        {
            lex_num_literal(&ls);
        }
        else if (c == '\n')
        {
            add_lex(&ls, LexTokenType::StatementEnd, ls.head, 1);
            ++ls.head;
        }
        else if (c == '\r' && ls.head < ls.end && *(ls.head + 1) == '\n')
        {
            add_lex(&ls, LexTokenType::StatementEnd, ls.head, 2);
            ls.head += 2;
        }
        else if (c == '\0')
        {
            add_lex(&ls, LexTokenType::EndOfFile, ls.head, 1);
            ++ls.head;
        }
        else if (c == ' ' || c == '\t')
        {
            ++ls.head;
        }

        Assert(ls.head > c_ptr, "Unknown characters detected by lexer.");
    }

    return ls.out_num;
}