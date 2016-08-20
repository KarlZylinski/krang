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

enum struct ParseNodeType
{
    Scope,
    FunctionDef,
    FunctionCall,
    Loop,
    VariableDecl,
    VariableAssignment
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

struct ParseFunctionDef
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

struct ParseVariableDecl
{
    DataType type;
    char* name;
    unsigned name_len;
    bool is_mutable;
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
    ParseNodeType type;

    union
    {
        ParseScope scope;
        ParseFunctionDef function_def;
        ParseFunctionCall function_call;
        ParseLoop loop;
        ParseVariableDecl variable_decl;
        ParseVariableAssignment variable_assignment;
    };
};

ParseScope parse(Allocator* alloc, const Token* lex_tokens, size_t num_lex_tokens);
