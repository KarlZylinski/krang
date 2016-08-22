#include "parser.h"
#include <string.h>
#include <stdlib.h>
#include "memory.h"
#include "tokenizer.h"

struct ParserState
{
    const Token* start;
    const Token* head;
    const Token* end;
    Allocator* allocator;
};

static DataType parse_type_name(ParserState* ps)
{
    Assert(ps->head->type == Token::Type::Name, "Error in parser: Tried to parse invalid type name.");
    if (str_equal(ps->head->val, "i32", ps->head->len))
    {
        ++ps->head;
        return DataType::Int32;
    }

    Error("Error in parser: Unknown datatype.");
    return (DataType)(-1);
}

static size_t num_tokens_diff(const Token* s, const Token* e)
{
    return mem_ptr_diff(s, e) / sizeof(Token);
}

static bool parse_func_def_check(ParserState* ps)
{
    return num_tokens_diff(ps->head, ps->end) >= 2
        && ps->head->type == Token::Type::Name
        && (ps->head + 1)->type == Token::Type::Name
        && (ps->head + 2)->type == Token::Type::ArgStart;
}

static void parse_scope(ParserState* ps, ParseScope* scope, bool close_on_statement_end = true);

static void parse_func_def(ParserState* ps, ParseScope* scope)
{
    Assert(parse_func_def_check(ps), "Error in parser: Invalid func def.");
    ParseNode* n = scope->nodes.push_init();
    n->type = ParseNode::Type::FunctionDefinition;
    ParseFunctionDefinition& pfd = n->function_definition;
    pfd.return_type = parse_type_name(ps);
    pfd.name = ps->head->val;
    pfd.name_len = ps->head->len;
    ++ps->head; // name
    ++ps->head; // arg start
    // TODO: READ ARGS
    ++ps->head; // arg end
    pfd.scope.nodes = dynamic_array_create<ParseNode>(ps->allocator);
    parse_scope(ps, &pfd.scope, false);
}

static Value parse_value(ParserState* ps)
{
    const Token& t = *ps->head;
    Value v = {};
    v.type = DataType::Int32;
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
        const Token& t = *ps->head;
        const Token* t_ptr = ps->head;

        switch (t.type)
        {
            case Token::Type::Literal:
                    parameters->add(parse_value(ps));
                break;
            case Token::Type::ArgStart:
                ++ps->head;
                break;
            case Token::Type::ArgEnd:
                ++ps->head;
                return;
            default:
                Error("Error in parser: Unknown function call argument type.");
                return;
        }

        Assert(ps->head > t_ptr, "Parser error: Func call parameters parser stuck.");
    }
}

static bool parse_func_call_check(ParserState* ps)
{
    return num_tokens_diff(ps->head, ps->end) >= 2
        && ps->head->type == Token::Type::Name
        && (ps->head + 1)->type == Token::Type::ArgStart;
}

static void parse_func_call(ParserState* ps, ParseScope* scope)
{
    Assert(parse_func_call_check(ps), "Error in parser: Invalid func call.");
    ParseNode* n = scope->nodes.push_init();
    n->type = ParseNode::Type::FunctionCall;
    ParseFunctionCall& pfc = n->function_call;
    pfc.name = ps->head->val;
    pfc.name_len = ps->head->len;
    ++ps->head;
    pfc.parameters = dynamic_array_create<Value>(ps->allocator);
    parse_func_call_parameters(ps, &pfc.parameters);
}

#define parse_loop_check (num_tokens_diff(ps->head, ps->end) >= 1 \
        && ps->head->type == Token::Type::Name \
        && memcmp(ps->head->val, "loop", ps->head->len) == 0)

static void parse_loop(ParserState* ps, ParseScope* scope)
{
    Assert(parse_loop_check, "Error in parser: Invalid loop.");
    ParseNode* n = scope->nodes.push_init();
    n->type = ParseNode::Type::Loop;
    ParseLoop& pl = n->loop;
    ++ps->head; // loop keyword
    ++ps->head; // opening brace! TODO: fix no-brace type
    pl.scope.nodes = dynamic_array_create<ParseNode>(ps->allocator);
    parse_scope(ps, &pl.scope);
}


static ParseExpression parse_expression(ParserState* ps)
{
    ParseExpression expr = {};
    expr.operand1 = parse_value(ps);

    // get statement end, remove later..
    ++ps->head;
    
    // TODO ADD STUFF HERE
    //if (ps->head->type == ParseNode::Type::StatementEnd)
        return expr;


}

static bool parse_variable_decl_check(ParserState* ps)
{
    return num_tokens_diff(ps->head, ps->end) >= 3
        && ps->head->type == Token::Type::Name
        && (memcmp(ps->head->val, "let", ps->head->len) == 0 || memcmp(ps->head->val, "mut", ps->head->len) == 0)
        && (ps->head + 1)->type == Token::Type::Name;
}

static void parse_variable_decl(ParserState* ps, ParseScope* scope)
{
    Assert(parse_variable_decl_check(ps), "Error in parser: Invalid variable decl.");
    ParseNode* n = scope->nodes.push_init();
    n->type = ParseNode::Type::VariableDeclaration;
    ParseVariableDeclaration& vd = n->variable_declaration;
    vd.is_mutable = memcmp(ps->head->val, "mut", ps->head->len) == 0;
    vd.has_initial_value = true;
    ++ps->head; // let/mut
    vd.name = ps->head->val;
    vd.name_len = ps->head->len;
    ++ps->head; // name
    ++ps->head; // assignment op
    vd.value_expr = parse_expression(ps);
    vd.type = vd.value_expr.operand1.type;
}

static bool is_variable_assignment(ParserState* ps)
{
    return num_tokens_diff(ps->head, ps->end) >= 2
        && ps->head->type == Token::Type::Name
        && (ps->head + 1)->type == Token::Type::Assignment;
}

static void parse_variable_assignment(ParserState* ps, ParseScope* scope)
{
    Assert(is_variable_assignment(ps), "Error in parser: Invalid variable assignment.");
    ParseNode* n = scope->nodes.push_init();
    n->type = ParseNode::Type::VariableAssignment;
    ParseVariableAssignment& va = n->variable_assignment;
    va.name = ps->head->val;
    va.name_len = ps->head->len;
    ++ps->head; // name done
    ++ps->head; // get rid of assignment op
    va.value_expr = parse_expression(ps);
}

static void parse_name_in_scope(ParserState* ps, ParseScope* scope)
{
    if (parse_func_def_check(ps))
    {
        parse_func_def(ps, scope);
    }
    else if (parse_func_call_check(ps))
    {
        parse_func_call(ps, scope);
    }
    else if (parse_loop_check)
    {
        parse_loop(ps, scope);
    }
    else if (parse_variable_decl_check(ps))
    {
        parse_variable_decl(ps, scope);
    }
    else if (is_variable_assignment(ps))
    {
        parse_variable_assignment(ps, scope);
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
        const Token& t = *ps->head;
        const Token* t_ptr = ps->head;

        switch (t.type)
        {
            case Token::Type::Name:
                parse_name_in_scope(ps, scope);
                break;
            case Token::Type::StatementEnd:
                ++ps->head;
                if (close_on_statement_end)
                    return;
                else
                    break;
            case Token::Type::ScopeStart:
                ++ps->head;
                close_on_statement_end = false;
                break;
            case Token::Type::ScopeEnd:
                Assert(close_on_statement_end == false, "Error in parser: Scope start end mismatch.");
                ++ps->head;
                return;
            case Token::Type::EndOfFile:
                Assert(close_on_statement_end == false, "Error in parser: Scope start end mismatch.");
                return;
        }

        Assert(ps->head > t_ptr, "Parser stuck.");
    }
}

ParseScope parse(Allocator* alloc, const Token* lex_tokens, size_t num_lex_tokens)
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