#pragma once
#include "data_type.h"
#include "parser.h"

enum struct LocalVariableStorageType
{
    Stack
    // Add registers here
};

struct AsmChunk;

struct LocalVariableData
{
    char* name;
    unsigned name_len;
    unsigned stack_offset; // only used for stack variables
    bool is_mutable;
    DataType type;
    LocalVariableStorageType storage_type;
};

struct AsmChunkVariableDeclarationData
{
    unsigned local_variable_index;
    bool has_initial_value;
    ParseExpression initial_value;
};

struct AsmChunkScopeData
{
    DynamicArray<AsmChunk> chunks;
};

struct AsmChunkFunctionDefinitionData
{
    char* name;
    unsigned name_len;
    DataType return_type;
    DynamicArray<LocalVariableData> local_variables;
    AsmChunkScopeData scope_data;
};

struct AsmChunkVariableAssignmentData
{
    unsigned local_variable_index;
    ParseExpression value;
};

struct AsmChunk
{
    enum struct Type
    {
        FunctionDefinition,
        Scope,
        ScopeEnd,
        VariableDeclaration,
        VariableAssignment,
        SecondPassParseNode
    };

    Type type;

    union
    {
        AsmChunkFunctionDefinitionData function_definition;
        AsmChunkVariableDeclarationData variable_declaration;
        AsmChunkVariableAssignmentData variable_assignment;
        ParseNode second_pass_parse_node;
    };
};
