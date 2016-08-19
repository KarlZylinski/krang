#pragma once

struct Allocator;

struct File
{
    unsigned char* data;
    unsigned size;
};

struct LoadedFile
{
    bool valid;
    File file;
};

LoadedFile file_load(Allocator* alloc, const char* filename);
bool file_write(void* data, unsigned size, const char* filename);
