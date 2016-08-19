#include "file.h"
#include "memory.h"
#include <stdio.h>

LoadedFile file_load(Allocator* alloc, const char* filename)
{
    FILE* file_handle = fopen(filename, "rb");

    if (file_handle == nullptr)
        return {false};

    fseek(file_handle, 0, SEEK_END);
    unsigned filesize = ftell(file_handle);
    fseek(file_handle, 0, SEEK_SET);

    if (filesize == 0)
        return {false};

    unsigned char* data = (unsigned char*)alloc->alloc(unsigned(filesize));

    if (data == nullptr)
        return {false};

    fread(data, 1, filesize, file_handle);
    fclose(file_handle);
    File file = {};
    file.data = data;
    file.size = filesize;
    return {true, file};
}

bool file_write(void* data, unsigned size, const char* filename)
{
    FILE* file_handle = fopen(filename, "wb");

    if (file_handle == nullptr)
        return false;

    fwrite(data, 1, size, file_handle);
    fclose(file_handle);
    return true;
}
