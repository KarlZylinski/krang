#include "lexer.h"
#include "memory.h"

struct LexerState
{
    char* start;
    char* head;
    char* end;
    LexToken* out;
    size_t out_num;
    void(*add_action)(LexerState*, LexTokenType, char*, unsigned);
};

static void add_lex(LexerState* ls, LexTokenType type, char* val, unsigned len)
{
    ls->add_action(ls, type, val, len);
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

static void run_lex(LexerState* ls)
{
    Assert(ls->add_action != nullptr, "Error in lexer: add_action in LexerState not set.");

    while (ls->head <= ls->end)
    {
        const char c = *ls->head;
        char* c_ptr = ls->head;

        switch (c)
        {
            case '(':
                add_lex(ls, LexTokenType::ArgStart, ls->head, 1);
                ++ls->head;
                break;
            case ')':
                add_lex(ls, LexTokenType::ArgEnd, ls->head, 1);
                ++ls->head;
                break;
            case '{':
                add_lex(ls, LexTokenType::ScopeStart, ls->head, 1);
                ++ls->head;
                break;
            case '}':
                add_lex(ls, LexTokenType::ScopeEnd, ls->head, 1);
                ++ls->head;
                break;
            case '=':
                add_lex(ls, LexTokenType::Assignment, ls->head, 1);
                ++ls->head;
                break;
            case '+':
                add_lex(ls, LexTokenType::Operator, ls->head, 1);
                ++ls->head;
                break;
            case '\n':
                add_lex(ls, LexTokenType::StatementEnd, ls->head, 1);
                ++ls->head;
                break;
            case '\0':
                add_lex(ls, LexTokenType::EndOfFile, ls->head, 1);
                ++ls->head;
                break;
            case ' ':
            case '\t':
                ++ls->head;
                break;
            default:
            {
                if (c >= 'a' && c <= 'z')
                {
                    lex_name(ls);
                }
                else if (c >= '0' && c <= '9')
                {
                    lex_num_literal(ls);
                }
                else if (c == '\r' && ls->head < ls->end && *(ls->head + 1) == '\n')
                {
                    add_lex(ls, LexTokenType::StatementEnd, ls->head, 2);
                    ls->head += 2;
                }
                else if (c == ' ' || c == '\t')
                {
                    ++ls->head;
                }
            } break;
        }

        Assert(ls->head > c_ptr, "Error in lexer: Unknown characters detected, stuck.");
    }
}

static void add_action_add(LexerState* ls, LexTokenType type, char* val, unsigned len)
{
    LexToken& t = ls->out[ls->out_num];
    t.type = type;
    t.val = val;
    t.len = len;
    ++ls->out_num;
}

static void add_action_count(LexerState* ls, LexTokenType type, char* val, unsigned len)
{
    ++ls->out_num;
}

LexerResult lex(char* data, unsigned size, Allocator* allocator)
{
    LexerState ls = {};
    ls.start = data;
    ls.head = ls.start;
    ls.end = data + size;
    ls.add_action = add_action_add;

    LexerState counter_state = ls;
    counter_state.add_action = add_action_count;

    run_lex(&counter_state);
    ls.out = (LexToken*)allocator->alloc_zero(sizeof(LexToken) * (unsigned)counter_state.out_num);
    run_lex(&ls);
    Assert(ls.out_num == counter_state.out_num, "Error in lexer: Counting lex pass and actual lexing pass generated different result.");

    LexerResult lr = {};
    lr.data = ls.out;
    lr.num = ls.out_num;
    return lr;
}