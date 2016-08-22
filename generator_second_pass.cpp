#include "generator_second_pass.h"
#include "generator.h"
#include "generator_first_pass.h"
#include "memory.h"

static void generate_for_scope(Allocator* allocator, DynamicArray<AsmChunk>* chunks, DynamicArray<LocalVariableData>* local_variables, const DynamicArray<AsmChunk>& scope);

static void generate_for_scope_end(Allocator* allocator, DynamicArray<AsmChunk>* chunks)
{
    AsmChunk* c = chunks->push_init();
    c->type = AsmChunk::Type::ScopeEnd;
}

static void generate_for_function_defintion(Allocator* allocator, DynamicArray<AsmChunk>* chunks, DynamicArray<LocalVariableData>* local_variables, const AsmChunkFunctionDefinitionData& fd)
{
    AsmChunk* c = chunks->push_init();
    c->type = AsmChunk::Type::FunctionDefinition;
    AsmChunkFunctionDefinitionData* fdd = &c->function_definition;
    memcpy(fdd, &fd, sizeof(AsmChunkFunctionDefinitionData));
    fdd->scope_data.chunks = dynamic_array_create<AsmChunk>(allocator);
    generate_for_scope(allocator, &fdd->scope_data.chunks, &fdd->local_variables, fd.scope_data.chunks);
    generate_for_scope_end(allocator, &fdd->scope_data.chunks);
}

static void generate_second_pass_chunk(Allocator* allocator, DynamicArray<AsmChunk>* chunks, DynamicArray<LocalVariableData>* local_variables, const ParseNode& pn)
{
    switch (pn.type)
    {
        case ParseNode::Type::FunctionCall:
        {
            const ParseFunctionCall& fc = pn.function_call;
            Assert(str_equal(fc.name, "ret", 3), "WIP");
            AsmChunk* c = chunks->push_init();
            c->type = AsmChunk::Type::Return;
            c->ret.value.op = ParseOperator::Literal;
            c->ret.value.operand1 = fc.parameters[0];
        } break;
        default:
            Error("Error in second pass generator: Missing second pass chunk generator.");
            break;
    }
}

static void generate_for_scope(Allocator* allocator, DynamicArray<AsmChunk>* chunks, DynamicArray<LocalVariableData>* local_variables, const DynamicArray<AsmChunk>& scope)
{
    for (unsigned i = 0; i < scope.num; ++i)
    {
        const AsmChunk& ac = scope[i];

        switch (ac.type)
        {
            case AsmChunk::Type::Scope:
                generate_for_scope(allocator, chunks, local_variables, ac.scope.chunks); // this is wrong, it needs it's own local variables?? also, shouldn't there be
                // just "variables in scope"-thing to get stuff from outside the scope?
                break;
            case AsmChunk::Type::FunctionDefinition:
                generate_for_function_defintion(allocator, chunks, local_variables, ac.function_definition);
                break;
            case AsmChunk::Type::VariableDeclaration:
            {
                AsmChunk* chunk = chunks->push_init();
                memcpy(chunk, &ac, sizeof(AsmChunk));
            } break;
            case AsmChunk::Type::VariableAssignment:
            {
                AsmChunk* chunk = chunks->push_init();
                memcpy(chunk, &ac, sizeof(AsmChunk));
            } break;
            case AsmChunk::Type::SecondPassParseNode:
                generate_second_pass_chunk(allocator, chunks, local_variables, ac.second_pass_parse_node);
                break;
            default:
            {
                Error("Error in second pass generator: Unknown asm chunk type.");
            } break;
        }
    }
}

GeneratedCodeSecondPass generate_second_pass(Allocator* allocator, const DynamicArray<AsmChunk>& first_pass)
{
    DynamicArray<AsmChunk> chunks = dynamic_array_create<AsmChunk>(allocator);
    generate_for_scope(allocator, &chunks, nullptr, first_pass);
    GeneratedCodeSecondPass gc = {};
    gc.chunks = chunks;
    return gc;
}
