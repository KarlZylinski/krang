#pragma once
#include <stdio.h>
#include "dynamic_array.h"

struct Allocator;
struct Token;

enum struct DataType
{
    Void,
    Int32
};

struct Value
{
    DataType type;

    union
    {
        int int32_literal_val;
        char* str_val;
    };

    unsigned str_val_len;
};

struct ParseNode;

struct ParseScope
{
    DynamicArray<ParseNode> nodes;
};

struct ParseFunctionDefinition
{
    DataType return_type;
    char* name;
    unsigned name_len;
    ParseScope scope;
};

struct ParseFunctionCall
{
    char* name;
    unsigned name_len;
    DynamicArray<Value> parameters;
};

struct ParseLoop
{
    ParseScope scope;
};

enum struct ParseOperator
{
    Literal,
    Plus
};

struct ParseExpression
{
    ParseOperator op;
    Value operand1;
    Value operand2;
};

struct ParseVariableDeclaration
{
    DataType type;
    char* name;
    unsigned name_len;
    bool is_mutable;
    bool has_initial_value;
    ParseExpression value_expr;
};

struct ParseVariableAssignment
{
    char* name;
    unsigned name_len;
    ParseExpression value_expr;
};


struct ParseNode
{
    enum struct Type
    {
        Scope,
        FunctionDefinition,
        FunctionCall,
        Loop,
        VariableDeclaration,
        VariableAssignment
    };

    Type type;

    union
    {
        ParseScope scope;
        ParseFunctionDefinition function_definition;
        ParseFunctionCall function_call;
        ParseLoop loop;
        ParseVariableDeclaration variable_declaration;
        ParseVariableAssignment variable_assignment;
    };
};

ParseScope parse(Allocator* alloc, const Token* lex_tokens, size_t num_lex_tokens);
