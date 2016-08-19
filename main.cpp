#include "memory.h"
#include <windows.h>
#include <stdio.h>
#include "file.h"
#include "lexer.h"
#include "parser.h"

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

    if (strlen(argv[1]) == 0)
    {
        printf("No input file specified.");
        return -1;
    }

    Allocator perma_alloc = create_permanent_allocator();
    LoadedFile lf = file_load(&perma_alloc, argv[1]);

    if (!lf.valid)
    {
        printf("Failed loading input file.");
        return -1;
    }

    Allocator heap_alloc = create_heap_allocator();
    LexToken* lex_tokens = (LexToken*)heap_alloc.alloc(lf.file.size * sizeof(LexToken));
    size_t num_lex_tokens = lex((char*)lf.file.data, lf.file.size, lex_tokens);
    ParseScope ps = parse(&heap_alloc, lex_tokens, num_lex_tokens);

    return 0;
}