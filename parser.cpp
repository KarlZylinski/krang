#include "parser.h"
#include <string.h>
#include <stdlib.h>
#include "memory.h"
#include "lexer.h"

struct ParserState
{
    const LexToken* start;
    const LexToken* head;
    const LexToken* end;
    Allocator* allocator;
};

static DataType parse_type_name(char* name, unsigned len)
{
    if (memcmp(name, "i32", len) == 0)
    {
        return DataType::Int32;
    }

    Error("Error in parser: Unknown datatype.");
    return (DataType)(-1);
}

#define parse_func_def_check (mem_ptr_diff(ps->head, ps->end) >= 3 \
        && ps->head->type == LexTokenType::Name \
        && (ps->head + 1)->type == LexTokenType::Name \
        && (ps->head + 2)->type == LexTokenType::ArgStart)

static void parse_scope(ParserState* ps, ParseScope* scope);

static void parse_func_def(ParserState* ps, ParseScope* scope)
{
    Assert(parse_func_def_check, "Error in parser: Invalid func def.");
    ParseNode* n = scope->nodes.push_init();
    n->type = ParseNodeType::FunctionDef;
    ParseFunctionDef& pfd = n->function_def;
    pfd.return_type = parse_type_name(ps->head->val, ps->head->len);
    ++ps->head;
    pfd.name = ps->head->val;
    pfd.name_len = ps->head->len;
    ++ps->head;
    pfd.scope.nodes = dynamic_array_create<ParseNode>(ps->allocator);
    ++ps->head;

    // Add arg parsing here.
    ++ps->head;
    ++ps->head;

    // For opening
    ++ps->head;
    parse_scope(ps, &pfd.scope);
}

static void parse_func_call_parameters(ParserState* ps, DynamicArray<Value>* parameters)
{
    while (ps->head < ps->end)
    {
        const LexToken& t = *ps->head;
        const LexToken* t_ptr = ps->head;

        switch (t.type)
        {
            case LexTokenType::Int32Literal:
                {
                    Value v = {};
                    v.type = ValueType::Int32Literal;
                    v.int32_literal_val = atoi(t.val);
                    v.str_val = t.val;
                    v.str_val_len = t.len;
                    parameters->add(v);
                    ++ps->head;
                }
                break;
            case LexTokenType::ArgEnd:
                ++ps->head;
                return;
            default:
                Error("Error in parser: Unknown function call argument type.");
                return;
        }

        Assert(ps->head > t_ptr, "Parser error: Func call parameters parser stuck.");
    }
}

#define parse_func_call_check (mem_ptr_diff(ps->head, ps->end) >= 2 \
        && ps->head->type == LexTokenType::Name \
        && (ps->head + 1)->type == LexTokenType::ArgStart)

static void parse_func_call(ParserState* ps, ParseScope* scope)
{
    Assert(parse_func_call_check, "Error in parser: Invalid func call.");
    ParseNode* n = scope->nodes.push_init();
    n->type = ParseNodeType::FunctionCall;
    ParseFunctionCall& pfc = n->function_call;
    pfc.name = ps->head->val;
    pfc.name_len = ps->head->len;
    ++ps->head;
    pfc.parameters = dynamic_array_create<Value>(ps->allocator);
    ++ps->head;
    parse_func_call_parameters(ps, &pfc.parameters);
}

static void parse_name_in_scope(ParserState* ps, ParseScope* scope)
{
    // maybe func def, minimum type, name and parentheses
    if (parse_func_def_check)
    {
        parse_func_def(ps, scope);
    }
    else if (parse_func_call_check)
    {
        parse_func_call(ps, scope);
    }
}

static void parse_scope(ParserState* ps, ParseScope* scope)
{
    while (ps->start < ps->end)
    {
        const LexToken& t = *ps->head;
        const LexToken* t_ptr = ps->head;

        switch (t.type)
        {
            case LexTokenType::Name:
                parse_name_in_scope(ps, scope);
                break;
            case LexTokenType::StatementEnd:
                ++ps->head;
                break;
            case LexTokenType::ScopeEnd:
                ++ps->head;
                return;
            case LexTokenType::EndOfFile:
                return;
        }

        Assert(ps->head > t_ptr, "Parser stuck.");
    }
}

ParseScope parse(Allocator* alloc, const LexToken* lex_tokens, size_t num_lex_tokens)
{
    ParserState ps = {};
    ps.start = lex_tokens;
    ps.head = ps.start;
    ps.end = ps.start + num_lex_tokens;
    ps.allocator = alloc;
    ParseScope root_scope = {};
    root_scope.nodes = dynamic_array_create<ParseNode>(alloc);
    parse_scope(&ps, &root_scope);
    return root_scope;
}