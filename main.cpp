#include "memory.h"
#include <windows.h>
#include <stdio.h>
#include "file.h"
#include "lexer.h"
#include "parser.h"
#include "code_generator.h"

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

    LexerResult lexer_result = lex((char*)lf.file.data, lf.file.size, &perma_alloc);
    ParseScope ps = parse(&perma_alloc, lexer_result.data, lexer_result.num);

    Allocator heap_alloc = create_heap_allocator();
    GeneratedCode cg = generate_code(&heap_alloc, ps);

    Allocator ta = create_temp_allocator();
    unsigned code_filename_len = (unsigned)strlen(filename) + 4;
    char* code_filename = (char*)ta.alloc(code_filename_len);
    strcpy(code_filename, filename);
    strcat(code_filename, ".asm");
    file_write(cg.code, cg.len, code_filename);
    heap_alloc.dealloc(cg.code);

    unsigned obj_filename_len = (unsigned)strlen(filename) + 4;
    char* obj_filename = (char*)ta.alloc(obj_filename_len);
    strcpy(obj_filename, filename);
    strcat(obj_filename, ".obj");

    const char* asm_format = "nasm -f win32 -o %s %s";
    unsigned asm_cmd_len = (unsigned)strlen(asm_format) + obj_filename_len + code_filename_len;
    char* asm_cmd = (char*)ta.alloc(asm_cmd_len);
    sprintf(asm_cmd, asm_format, obj_filename, code_filename);
    system(asm_cmd);

    const char* link_format = "golink %s";
    unsigned link_cmd_len = (unsigned)strlen(link_format) + obj_filename_len;
    char* link_cmd = (char*)ta.alloc(link_cmd_len);
    sprintf(link_cmd, link_format, obj_filename);
    system(link_cmd);
   
    heap_allocator_check_clean(&heap_alloc);

    return 0;
}