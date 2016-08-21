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

void generate_local_variables_for_function(DynamicArray<AsmChunk>* chunks, DynamicArray<LocalVariableData>* local_variables, const ParseScope& ps)
{
    unsigned offset = 0;
    for (unsigned i = 0; i < ps.nodes.num; ++i)
    {
        const ParseNode& pn = ps.nodes[i];

        switch (pn.type)
        {
            case ParseNode::Type::VariableDeclaration:
            {
                const ParseVariableDeclaration& vd = pn.variable_declaration;
                LocalVariableData* lvd = local_variables->push_init();
                lvd->name = vd.name;
                lvd->name_len = vd.name_len;
                lvd->stack_offset = offset;
                offset += data_type_size(vd.type);
                lvd->type = vd.type;
                lvd->storage_type = LocalVariableStorageType::Stack;
                lvd->is_mutable = vd.is_mutable;
                AsmChunk* chunk = chunks->push_init();
                chunk->type = AsmChunk::Type::VariableDeclaration;
                AsmChunkVariableDeclarationData& cvd = chunk->variable_declaration;
                cvd.data = lvd;
                cvd.has_initial_value = vd.has_initial_value;
                cvd.initial_value = vd.value_expr;
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

void generate_for_function_defintion(Allocator* allocator, DynamicArray<AsmChunk>* chunks, const ParseFunctionDefinition& fd)
{
    AsmChunk* c = chunks->push_init();
    c->type = AsmChunk::Type::FunctionDefinition;
    AsmChunkFunctionDefinitionData& fdd = c->function_definition;
    fdd.return_type = fd.return_type;
    fdd.name = fd.name;
    fdd.name_len = fd.name_len;
    fdd.local_variables = dynamic_array_create<LocalVariableData>(allocator);
    fdd.scope_data.chunks = dynamic_array_create<AsmChunk>(allocator);
    generate_local_variables_for_function(&fdd.scope_data.chunks, &fdd.local_variables, fd.scope);
}

void generate_for_scope(Allocator* allocator, DynamicArray<AsmChunk>* chunks, const ParseScope& ps)
{
    for (unsigned i = 0; i < ps.nodes.num; ++i)
    {
        const ParseNode& pn = ps.nodes[i];

        switch (pn.type)
        {
            case ParseNode::Type::Scope:
                generate_for_scope(allocator, chunks, pn.scope);
                break;
            case ParseNode::Type::FunctionDefinition:
                generate_for_function_defintion(allocator, chunks, pn.function_definition);
                break;
        }
    }
}

GeneratedCodeFirstPass generate_first_pass(Allocator* allocator, const ParseScope& ps)
{
    DynamicArray<AsmChunk> chunks = dynamic_array_create<AsmChunk>(allocator);
    generate_for_scope(allocator, &chunks, ps);
    GeneratedCodeFirstPass gc = {};
    gc.chunks = chunks.data;
    gc.num_chunks = chunks.num;
    return gc;
}
