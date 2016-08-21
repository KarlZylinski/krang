#include "memory.h"
#include <windows.h>
#include <stdio.h>
#include "file.h"
#include "tokenizer.h"
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
    DataType type;
    LocalVariableStorageType storage_type;
};

struct AsmChunkVariableDeclarationData
{
    LocalVariableData* data;
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
    LocalVariableData* data;
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

struct GeneratedCodeFirstPass
{
    AsmChunk* chunks;
    unsigned num_chunks;
};

GeneratedCodeFirstPass generate_first_pass(Allocator* allocator, const ParseScope& ps)
{
    DynamicArray<AsmChunk> chunks = dynamic_array_create<AsmChunk>(allocator);
    generate_for_scope(allocator, &chunks, ps);
    GeneratedCodeFirstPass gc = {};
    gc.chunks = chunks.data;
    gc.num_chunks = chunks.num;
    return gc;
}

const static char* usage_string = "Usage: krang.exe input.kra";

int main(int argc, char** argv)
{
    void* temp_memory_block = VirtualAlloc(nullptr, TempMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    Assert(temp_memory_block != nullptr, "Failed allocating temp memory.");
    temp_memory_blob_init(temp_memory_block, TempMemorySize);

    void* permanent_memory_block = VirtualAlloc(nullptr, PermanentMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    Assert(permanent_memory_block != nullptr, "Failed allocating permanent memory.");
    permanent_memory_blob_init(permanent_memory_block, PermanentMemorySize);

    if (argc != 2)
    {
        printf(usage_string);
        return -1;
    }

    char* filename = argv[1];

    if (strlen(filename) == 0)
    {
        printf("No input file specified.");
        return -1;
    }

    Allocator perma_alloc = create_permanent_allocator();
    LoadedFile lf = file_load(&perma_alloc, filename);

    if (!lf.valid)
    {
        printf("Failed loading input file.");
        return -1;
    }

    TokenizerResult tokenizer_result = tokenize((char*)lf.file.data, lf.file.size, &perma_alloc);
    ParseScope ps = parse(&perma_alloc, tokenizer_result.data, tokenizer_result.num);

    Allocator heap_alloc = create_heap_allocator();
    GeneratedCodeFirstPass cg = generate_first_pass(&heap_alloc, ps);
    (void)cg;
/*    Allocator ta = create_temp_allocator();
    size_t code_filename_len = strlen(filename) + 4;
    char* code_filename = (char*)ta.alloc(code_filename_len);
    strcpy(code_filename, filename);
    strcat(code_filename, ".asm");
    file_write(cg.code, cg.len, code_filename);
    heap_alloc.dealloc(cg.code);

    size_t obj_filename_len = strlen(filename) + 4;
    char* obj_filename = (char*)ta.alloc(obj_filename_len);
    strcpy(obj_filename, filename);
    strcat(obj_filename, ".obj");

    const char* asm_format = "nasm -f win32 -o %s %s";
    size_t asm_cmd_len = strlen(asm_format) + obj_filename_len + code_filename_len;
    char* asm_cmd = (char*)ta.alloc(asm_cmd_len);
    sprintf(asm_cmd, asm_format, obj_filename, code_filename);
    system(asm_cmd);

    const char* link_format = "golink %s";
    size_t link_cmd_len = strlen(link_format) + obj_filename_len;
    char* link_cmd = (char*)ta.alloc(link_cmd_len);
    sprintf(link_cmd, link_format, obj_filename);
    system(link_cmd);
   
    heap_allocator_check_clean(&heap_alloc);*/

    return 0;
}