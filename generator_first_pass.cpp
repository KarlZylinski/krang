#include "generator_first_pass.h"
#include "generator.h"
#include "memory.h"

static unsigned data_type_size(DataType type)
{
    switch(type)
    {
        case DataType::Int32:
            return 4;
            break;
    }

    Error("No size set for data type.");
    return 0;
}

static unsigned get_variable_declaration_index(LocalVariableData* local_variables, unsigned num_variables, char* name, unsigned name_len)
{
    for (unsigned i = 0; i < num_variables; ++i)
    {
        const LocalVariableData& lvd = local_variables[i];

        if (str_equal(lvd.name, name, name_len))
        {
            return i;
        }
    }

    Error("Error in generator: Failed finding variable declaration.");
    return 0;
}

static void generate_for_scope(Allocator* allocator, DynamicArray<AsmChunk>* chunks, DynamicArray<LocalVariableData>* local_variables, const ParseScope& ps);

static void generate_for_function_defintion(Allocator* allocator, DynamicArray<AsmChunk>* chunks, DynamicArray<LocalVariableData>* local_variables, const ParseFunctionDefinition& fd)
{
    AsmChunk* c = chunks->push_init();
    c->type = AsmChunk::Type::FunctionDefinition;
    AsmChunkFunctionDefinitionData& fdd = c->function_definition;
    fdd.return_type = fd.return_type;
    fdd.name = fd.name;
    fdd.name_len = fd.name_len;
    fdd.local_variables = dynamic_array_create<LocalVariableData>(allocator);
    fdd.scope_data.chunks = dynamic_array_create<AsmChunk>(allocator);
    generate_for_scope(allocator, &fdd.scope_data.chunks, &fdd.local_variables, fd.scope);
}

static void generate_for_scope(Allocator* allocator, DynamicArray<AsmChunk>* chunks, DynamicArray<LocalVariableData>* local_variables, const ParseScope& ps)
{
    unsigned local_variables_offset = 0;
    for (unsigned i = 0; i < ps.nodes.num; ++i)
    {
        const ParseNode& pn = ps.nodes[i];

        switch (pn.type)
        {
            case ParseNode::Type::Scope:
                generate_for_scope(allocator, chunks, local_variables, pn.scope); // this is wrong, it needs it's own local variables?? also, shouldn't there be
                // just "variables in scope"-thing to get stuff from outside the scope?
                break;
            case ParseNode::Type::FunctionDefinition:
                generate_for_function_defintion(allocator, chunks, local_variables, pn.function_definition);
                break;
            case ParseNode::Type::VariableDeclaration:
            {
                const ParseVariableDeclaration& vd = pn.variable_declaration;
                unsigned lvi = local_variables->num;
                LocalVariableData* lvd = local_variables->push_init();
                lvd->name = vd.name;
                lvd->name_len = vd.name_len;
                lvd->stack_offset = local_variables_offset;
                local_variables_offset += data_type_size(vd.type);
                lvd->type = vd.type;
                lvd->storage_type = LocalVariableStorageType::Stack;
                lvd->is_mutable = vd.is_mutable;
                AsmChunk* chunk = chunks->push_init();
                chunk->type = AsmChunk::Type::VariableDeclaration;
                AsmChunkVariableDeclarationData& cvd = chunk->variable_declaration;
                cvd.local_variable_index = lvi;
                cvd.has_initial_value = vd.has_initial_value;
                cvd.initial_value = vd.value_expr;
            } break;
            case ParseNode::Type::VariableAssignment:
            {
                const ParseVariableAssignment& va = pn.variable_assignment;
                unsigned lvi = get_variable_declaration_index(local_variables->data, local_variables->num, va.name, va.name_len);
                AsmChunk* chunk = chunks->push_init();
                chunk->type = AsmChunk::Type::VariableAssignment;
                AsmChunkVariableAssignmentData& vad = chunk->variable_assignment;
                vad.local_variable_index = lvi;
                vad.value = va.value_expr;
            } break;
            default:
            {
                AsmChunk* chunk = chunks->push_init();
                chunk->type = AsmChunk::Type::SecondPassParseNode;
                chunk->second_pass_parse_node = pn;
            } break;
        }
    }
}

GeneratedCodeFirstPass generate_first_pass(Allocator* allocator, const ParseScope& ps)
{
    DynamicArray<AsmChunk> chunks = dynamic_array_create<AsmChunk>(allocator);
    generate_for_scope(allocator, &chunks, nullptr, ps);
    GeneratedCodeFirstPass gc = {};
    gc.chunks = chunks;
    return gc;
}
