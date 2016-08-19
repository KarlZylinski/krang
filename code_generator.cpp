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
