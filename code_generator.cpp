#include "code_generator.h"
#include "memory.h"
#include <string.h>
#include "parser.h"

struct CodeGeneratorState
{
    char* out;
    unsigned len;
    unsigned cap;
    Allocator* allocator;
};

static void grow(CodeGeneratorState* cs, unsigned min_size)
{
    char* old_out = cs->out;
    unsigned new_cap = cs->cap == 0 ? min_size * 2 : min_size + cs->cap * 2;
    cs->out = (char*)cs->allocator->alloc(new_cap);
    cs->cap = new_cap;
    memcpy(cs->out, old_out, cs->len);
    cs->allocator->dealloc(old_out);
}

static void add_code(CodeGeneratorState* cs, const char* code, unsigned len)
{
    if (cs->len + len > cs->cap)
        grow(cs, len);

    memcpy(cs->out + cs->len, code, len);
    cs->len += len;
}

static void generate_scope(CodeGeneratorState* cs, const ParseScope& ps);

const static char* prologue =
    "push ebp\n"
    "mov ebp, esp\n";
const static unsigned prologue_len = (unsigned)strlen(prologue);

const static char* epilogue =
    "\nmov esp, ebp\n"
    "pop ebp\n";
const static unsigned epilogue_len = (unsigned)strlen(epilogue);

static void generate_function_def(CodeGeneratorState* cs, const ParseFunctionDef& def)
{
    add_code(cs, def.name, def.name_len);
    add_code(cs, ":\n", 1);
    add_code(cs, prologue, prologue_len);
    generate_scope(cs, def.scope);
}

static void generate_function_call(CodeGeneratorState* cs, const ParseFunctionCall& call)
{
    if (memcmp(call.name, "ret", call.name_len) == 0)
    {
        Assert(call.parameters.num <= 1, "Error in code generator: More than 1 return value.");
        const static char* ret_mov =
            "mov eax, ";
        const static unsigned ret_mov_len = (unsigned)strlen(ret_mov);

        add_code(cs, ret_mov, ret_mov_len);
        add_code(cs, call.parameters[0].str_val, call.parameters[0].str_val_len);
        add_code(cs, epilogue, epilogue_len);

        const static char* ret = "ret\n";
        const static unsigned ret_len = (unsigned)strlen(ret);
        
        add_code(cs, ret, ret_len);
        return;
    }

    Error("Error in code generator: Unknown function call type.");
}

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
    snprintf(buf, sizeof(buf_size), "%u", num);
    return buf;
}

static const char* reserve_space_for_var = "add esp, ";
static const unsigned reserve_space_for_var_len = (unsigned)strlen(reserve_space_for_var);

static const char* mov_init_val = "\nmov dword [ebp- ";
static const unsigned mov_init_val_len = (unsigned)strlen(mov_init_val);

static void generate_variable_decl(CodeGeneratorState* cs, const ParseVariableDecl& decl)
{
    add_code(cs, reserve_space_for_var, reserve_space_for_var_len);
    char* size_str = uint32_to_str(data_type_size(decl.type));
    unsigned size_str_len = (unsigned)strlen(size_str);
    add_code(cs, size_str, size_str_len);
    add_code(cs, mov_init_val, mov_init_val_len);
    add_code(cs, size_str, size_str_len);
    add_code(cs, "], ", 2);
    add_code(cs, decl.value_expr.operand1.str_val, decl.value_expr.operand1.str_val_len);
    add_code(cs, "\n", 1);
}

static void generate_variable_assignment(CodeGeneratorState* cs, const ParseVariableAssignment& assign)
{
    char* size_str = uint32_to_str(4);
    unsigned size_str_len = (unsigned)strlen(size_str);
    add_code(cs, mov_init_val, mov_init_val_len);
    add_code(cs, size_str, size_str_len);
    add_code(cs, "], ", 2);
    add_code(cs, assign.value_expr.operand1.str_val, assign.value_expr.operand1.str_val_len);
    add_code(cs, "\n", 1);
}

static void generate_scope(CodeGeneratorState* cs, const ParseScope& ps)
{
    for (unsigned i = 0; i < ps.nodes.num; ++i)
    {
        const ParseNode& pn = ps.nodes[i];

        switch (pn.type)
        {
            case ParseNodeType::FunctionDef:
                generate_function_def(cs, pn.function_def);
                break;

            case ParseNodeType::FunctionCall:
                generate_function_call(cs, pn.function_call);
                break;

            case ParseNodeType::VariableDecl:
                generate_variable_decl(cs, pn.variable_decl);
                break;

            case ParseNodeType::VariableAssignment:
                generate_variable_assignment(cs, pn.variable_assignment);
                break;

            default:
                Error("Error in code generator: Unknown parse node type.");
                return;
        }
    }
}

GeneratedCode generate_code(Allocator* alloc, const ParseScope& ps)
{
    CodeGeneratorState cs = {};
    cs.allocator = alloc;
    static const char* code_sect = "section .code\n";
    add_code(&cs, code_sect, (unsigned)strlen(code_sect));
    generate_scope(&cs, ps);
    GeneratedCode gc = {};
    gc.code = cs.out;
    gc.len = cs.len;
    return gc;
}
