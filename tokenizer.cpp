#include "tokenizer.h"
#include "memory.h"

struct TokenizerState
{
    char* start;
    char* head;
    char* end;
    Token* out;
    size_t out_num;
    void(*add_action)(TokenizerState*, Token::Type, char*, unsigned);
};

static void add_token(TokenizerState* ts, Token::Type type, char* val, unsigned len)
{
    ts->add_action(ts, type, val, len);
}

static void tokenize_name(TokenizerState* ts)
{
    char* val = ts->head;

    while (*ts->head >= 'a' && *ts->head <= 'z' || *ts->head >= '0' && *ts->head <= '9')
        ++ts->head;

    add_token(ts, Token::Type::Name, val, (unsigned)mem_ptr_diff(val, ts->head));
}

static void tokenize_num_literal(TokenizerState* ts)
{
    char* val = ts->head;

    while (*ts->head >= '0' && *ts->head <= '9')
        ++ts->head;

    add_token(ts, Token::Type::Literal, val, (unsigned)mem_ptr_diff(val, ts->head));
}

static void run_tokenization(TokenizerState* ts)
{
    Assert(ts->add_action != nullptr, "Error in tokoenizer: add_action in TokenizerState not set.");

    while (ts->head <= ts->end)
    {
        const char c = *ts->head;
        char* c_ptr = ts->head;

        switch (c)
        {
            case '(':
                add_token(ts, Token::Type::ArgStart, ts->head, 1);
                ++ts->head;
                break;
            case ')':
                add_token(ts, Token::Type::ArgEnd, ts->head, 1);
                ++ts->head;
                break;
            case '{':
                add_token(ts, Token::Type::ScopeStart, ts->head, 1);
                ++ts->head;
                break;
            case '}':
                add_token(ts, Token::Type::ScopeEnd, ts->head, 1);
                ++ts->head;
                break;
            case '=':
                add_token(ts, Token::Type::Assignment, ts->head, 1);
                ++ts->head;
                break;
            case '+':
                add_token(ts, Token::Type::Operator, ts->head, 1);
                ++ts->head;
                break;
            case '\n':
                add_token(ts, Token::Type::StatementEnd, ts->head, 1);
                ++ts->head;
                break;
            case '\0':
                add_token(ts, Token::Type::EndOfFile, ts->head, 1);
                ++ts->head;
                break;
            case '#':
                while (*ts->head != '\n' && !(*ts->head == '\r' && ts->head < ts->end && *(ts->head + 1) == '\n'))
                    ++ts->head;
                break;
            case ' ':
            case '\t':
                while (*ts->head == ' ' || *ts->head == '\t')
                    ++ts->head;
                break;
            default:
            {
                if (c >= 'a' && c <= 'z')
                {
                    tokenize_name(ts);
                }
                else if (c >= '0' && c <= '9')
                {
                    tokenize_num_literal(ts);
                }
                else if (ts->head < ts->end && c == '-' && *(ts->head + 1) == '>')
                {
                    add_token(ts, Token::Type::Arrow, ts->head, 2);
                    ts->head += 2;
                }
                else if (ts->head < ts->end && c == '\r' && *(ts->head + 1) == '\n')
                {
                    add_token(ts, Token::Type::StatementEnd, ts->head, 2);
                    ts->head += 2;
                }
                else if (c == ' ' || c == '\t')
                {
                    ++ts->head;
                }
            } break;
        }

        Assert(ts->head > c_ptr, "Error in tokoenizer: Unknown characters detected, stuck.");
    }
}

static void add_action_add(TokenizerState* ts, Token::Type type, char* val, unsigned len)
{
    Token& t = ts->out[ts->out_num];
    t.type = type;
    t.val = val;
    t.len = len;
    ++ts->out_num;
}

static void add_action_count(TokenizerState* ts, Token::Type type, char* val, unsigned len)
{
    ++ts->out_num;
}

TokenizerResult tokenize(char* data, size_t size, Allocator* allocator)
{
    TokenizerState ts = {};
    ts.start = data;
    ts.head = ts.start;
    ts.end = data + size;
    ts.add_action = add_action_add;

    TokenizerState counter_state = ts;
    counter_state.add_action = add_action_count;

    run_tokenization(&counter_state);
    ts.out = (Token*)allocator->alloc_zero(sizeof(Token) * (unsigned)counter_state.out_num);
    run_tokenization(&ts);
    Assert(ts.out_num == counter_state.out_num, "Error in tokoenizer: Counting lex pass and actual lexing pass generated different result.");

    TokenizerResult tr = {};
    tr.data = ts.out;
    tr.num = ts.out_num;
    return tr;
}