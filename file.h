#pragma once

struct Allocator;

struct File
{
    unsigned char* data;
    size_t size;
};

struct LoadedFile
{
    bool valid;
    File file;
};

LoadedFile file_load(Allocator* alloc, const char* filename);
bool file_write(void* data, size_t size, const char* filename);
