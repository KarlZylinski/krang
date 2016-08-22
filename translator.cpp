#include "translator.h"
#include "memory.h"
#include "generator.h"

struct AsmTranslationState
{
    char* out;
    size_t len;
    size_t cap;
    Allocator* allocator;
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

static char* uint32_to_str(unsigned num)
{
    const unsigned buf_size = 4096;
    static char buf[buf_size];
    sprintf(buf, "%u", num);
    return buf;
}

static void grow(AsmTranslationState* ts, size_t min_size)
{
    char* old_out = ts->out;
    size_t new_cap = ts->cap == 0 ? min_size * 2 : min_size + ts->cap * 2;
    ts->out = (char*)ts->allocator->alloc(new_cap);
    ts->cap = new_cap;
    memcpy(ts->out, old_out, ts->len);
    ts->allocator->dealloc(old_out);
}

static void add_code(AsmTranslationState* ts, const char* code, size_t len)
{
    if (ts->len + len > ts->cap)
        grow(ts, len);

    memcpy(ts->out + ts->len, code, len);
    ts->len += len;
}

static void translate_scope(AsmTranslationState* ts, const DynamicArray<LocalVariableData>* local_variables, const DynamicArray<AsmChunk>& chunks);

static void translate_function_definition(AsmTranslationState* ts, const DynamicArray<LocalVariableData>* local_variables, const AsmChunkFunctionDefinitionData& fd)
{
    const static char* prologue =
        "push ebp\n"
        "mov ebp, esp\n";
    const static size_t prologue_len = strlen(prologue);

    add_code(ts, fd.name, fd.name_len);
    add_code(ts, ":\n", 2);
    add_code(ts, prologue, prologue_len);

    unsigned local_variables_size = 0;
    const DynamicArray<LocalVariableData>& variables = fd.local_variables;
    for (unsigned i = 0; i < variables.num; ++i)
    {
        local_variables_size += data_type_size(variables[i].type);
    }

    if (local_variables_size > 0)
    {
        static const char* reserve_space_for_var = "add esp, ";
        static const size_t reserve_space_for_var_len = strlen(reserve_space_for_var);
        char* size_str = uint32_to_str(local_variables_size);
        size_t size_str_len = strlen(size_str);
        add_code(ts, reserve_space_for_var, reserve_space_for_var_len);
        add_code(ts, size_str, size_str_len);
        add_code(ts, "\n", 1);
    }

    translate_scope(ts, &fd.local_variables, fd.scope_data.chunks);

    const static char* epilogue =
        "mov esp, ebp\n"
        "pop ebp\n";
    const static size_t epilogue_len = strlen(epilogue);

    add_code(ts, epilogue, epilogue_len);
    add_code(ts, "ret", 3);
}

static const char* mov_init_val = "mov dword [ebp-";
static const size_t mov_init_val_len = strlen(mov_init_val);

static void translate_variable_declaration(AsmTranslationState* ts, const DynamicArray<LocalVariableData>* local_variables, const AsmChunkVariableDeclarationData& vd)
{
    Assert(vd.local_variable_index < local_variables->num, "Error on translator: Local variable index in variable declaration is out of bounds.");

    if (!vd.has_initial_value)
        return;

    const LocalVariableData& lvd = (*local_variables)[vd.local_variable_index];
    char* stack_offset_str = uint32_to_str(lvd.stack_offset);
    size_t stack_offset_str_len = strlen(stack_offset_str);
    add_code(ts, mov_init_val, mov_init_val_len);
    add_code(ts, stack_offset_str, stack_offset_str_len);
    add_code(ts, "], ", 2);
    add_code(ts, vd.initial_value.operand1.str_val, vd.initial_value.operand1.str_val_len);
    add_code(ts, "\n", 1);
}

static void translate_return(AsmTranslationState* ts, const DynamicArray<LocalVariableData>* local_variables, const AsmChunkReturnData& ret)
{
    add_code(ts, "mov eax, ", strlen("mov eax, "));
    add_code(ts, ret.value.operand1.str_val, ret.value.operand1.str_val_len);
    add_code(ts, "\n", 1);
}

static void translate_variable_assignment(AsmTranslationState* ts, const DynamicArray<LocalVariableData>* local_variables, const AsmChunkVariableAssignmentData& ad)
{
    Assert(ad.local_variable_index < local_variables->num, "Error on translator: Local variable index in variable assignment is out of bounds.");
    const LocalVariableData& lvd = (*local_variables)[ad.local_variable_index];
    char* stack_offset_str = uint32_to_str(lvd.stack_offset);
    size_t stack_offset_str_len = strlen(stack_offset_str);
    add_code(ts, mov_init_val, mov_init_val_len);
    add_code(ts, stack_offset_str, stack_offset_str_len);
    add_code(ts, "], ", 2);
    add_code(ts, ad.value.operand1.str_val, ad.value.operand1.str_val_len);
    add_code(ts, "\n", 1);
}

static void translate_scope(AsmTranslationState* ts, const DynamicArray<LocalVariableData>* local_variables, const DynamicArray<AsmChunk>& chunks)
{
    for (unsigned i = 0; i < chunks.num; ++i)
    {
        const AsmChunk& a = chunks[i];

        switch (a.type)
        {
            case AsmChunk::Type::FunctionDefinition:
                translate_function_definition(ts, local_variables, a.function_definition);
                break;
            case AsmChunk::Type::Return:
                translate_return(ts, local_variables, a.ret);
                break;
            case AsmChunk::Type::VariableDeclaration:
                translate_variable_declaration(ts, local_variables, a.variable_declaration);
                break;
            case AsmChunk::Type::VariableAssignment:
                translate_variable_assignment(ts, local_variables, a.variable_assignment);
                break;
            case AsmChunk::Type::ScopeEnd:
                return;
            default:
                Error("Unknown asm chunk type in asm translation.");
                break;
        }
    }
}

AsmTranslationResult translate_to_asm(Allocator* allocator, const DynamicArray<AsmChunk>& chunks)
{
    AsmTranslationState ts = {};
    ts.allocator = allocator;
    static const char* section_text = "section .text\n";
    static const size_t section_text_len = strlen(section_text);
    add_code(&ts, section_text, section_text_len);
    translate_scope(&ts, nullptr, chunks);
    AsmTranslationResult tr = {};
    tr.data = ts.out;
    tr.len = ts.len;
    return tr;
}
