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

static unsigned num_tokens_diff(const LexToken* s, const LexToken* e)
{
    return mem_ptr_diff(s, e) / sizeof(LexToken);
}

#define parse_func_def_check (num_tokens_diff(ps->head, ps->end) >= 3 \
        && ps->head->type == LexTokenType::Name \
        && (ps->head + 1)->type == LexTokenType::Name \
        && (ps->head + 2)->type == LexTokenType::ArgStart)

static void parse_scope(ParserState* ps, ParseScope* scope, bool close_on_statement_end = true);

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
    parse_scope(ps, &pfd.scope);
}

static Value parse_value(ParserState* ps)
{
    const LexToken& t = *ps->head;
    Value v = {};
    v.type = ValueType::Int32Literal;
    v.int32_literal_val = atoi(t.val);
    v.str_val = t.val;
    v.str_val_len = t.len;
    ++ps->head;
    return v;
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
                    parameters->add(parse_value(ps));
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

#define parse_func_call_check (num_tokens_diff(ps->head, ps->end) >= 2 \
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

#define parse_loop_check (num_tokens_diff(ps->head, ps->end) >= 1 \
        && ps->head->type == LexTokenType::Name \
        && memcmp(ps->head->val, "loop", ps->head->len) == 0)

static void parse_loop(ParserState* ps, ParseScope* scope)
{
    Assert(parse_loop_check, "Error in parser: Invalid loop.");
    ParseNode* n = scope->nodes.push_init();
    n->type = ParseNodeType::Loop;
    ParseLoop& pl = n->loop;
    ++ps->head; // loop keyword
    ++ps->head; // opening brace! TODO: fix no-brace type
    pl.scope.nodes = dynamic_array_create<ParseNode>(ps->allocator);
    parse_scope(ps, &pl.scope);
}

#define parse_variable_decl_check (num_tokens_diff(ps->head, ps->end) >= 3 \
        && ps->head->type == LexTokenType::Name \
        && (ps->head + 1)->type == LexTokenType::Name \
        && (ps->head + 2)->type != LexTokenType::ArgStart) // To not confuse with function decls.

static ParseExpression parse_expression(ParserState* ps)
{
    ParseExpression expr = {};
    expr.operand1 = parse_value(ps);

    // get statement end, remove later..
    ++ps->head;
    
    // TODO ADD STUFF HERE
    //if (ps->head->type == ParseNodeType::StatementEnd)
        return expr;


}

static void parse_variable_decl(ParserState* ps, ParseScope* scope)
{
    Assert(parse_variable_decl_check, "Error in parser: Invalid variable decl.");
    ParseNode* n = scope->nodes.push_init();
    n->type = ParseNodeType::VariableDecl;
    ParseVariableDecl& vd = n->variable_decl;
    vd.type = parse_type_name(ps->head->val, ps->head->len);
    ++ps->head; // type done
    vd.name = ps->head->val;
    vd.name_len = ps->head->len;
    ++ps->head; // name done
    ++ps->head; // get rid of assignment op
    vd.value_expr = parse_expression(ps);
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
    else if (parse_loop_check)
    {
        parse_loop(ps, scope);
    }
    else if (parse_variable_decl_check)
    {
        parse_variable_decl(ps, scope);
    }
    else
    {
        Error("Unknown name.");
    }
}

static void parse_scope(ParserState* ps, ParseScope* scope, bool close_on_statement_end)
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
                if (close_on_statement_end)
                    return;
                else
                    break;
            case LexTokenType::ScopeStart:
                ++ps->head;
                close_on_statement_end = false;
                return;
            case LexTokenType::ScopeEnd:
                Assert(close_on_statement_end == false, "Error in parser: Scope start end mismatch.");
                ++ps->head;
                return;
            case LexTokenType::EndOfFile:
                Assert(close_on_statement_end == false, "Error in parser: Scope start end mismatch.");
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
    parse_scope(&ps, &root_scope, false);
    return root_scope;
}