#pragma once

#include <string.h>

// This will eff your performance so only enable when debugging memory leaks in the heap allocator.
// #define ENABLE_MEMORY_TRACING

#if defined(ENABLE_MEMORY_TRACING)
    #include "callstack_capturer.h"
#endif

const unsigned DefaultMemoryAlign = 8;

struct Allocator
{
    ~Allocator()
    {
        if (out_of_scope)
            out_of_scope(this);
    }

    void* alloc(size_t size, unsigned align = DefaultMemoryAlign)
    {
        return alloc_internal(this, size, align);
    }

    void* alloc_zero(size_t size, unsigned align = DefaultMemoryAlign)
    {
        void* p = alloc_internal(this, size, align);

        if (p != nullptr)
            memset(p, 0, size);

        return p;
    }

    void dealloc(void* ptr)
    {
        dealloc_internal(this, ptr);
    }

    void*(*alloc_internal)(Allocator* alloc, size_t size, unsigned align);
    void(*dealloc_internal)(Allocator* alloc, void* ptr);
    void(*out_of_scope)(Allocator* alloc);
    void* last_alloc;
    unsigned num_allocations;

    #if defined(ENABLE_MEMORY_TRACING)
        CapturedCallstack* captured_callstacks;
    #endif
};

size_t mem_ptr_diff(const void* ptr1, const void* ptr2);
void* mem_ptr_add(const void* ptr1, size_t offset);
void* mem_ptr_sub(const void* ptr1, size_t offset);
void* mem_align_forward(const void* p, unsigned align);

void permanent_memory_blob_init(void* start, size_t capacity);
const size_t PermanentMemorySize = 32 * 1024 * 1024;
void* permanent_alloc(size_t size, unsigned align = DefaultMemoryAlign);

const size_t TempMemorySize = 1024 * 1024 * 1024;
void temp_memory_blob_init(void* start, size_t capacity);
size_t temp_memory_used();
void* temp_allocator_alloc(Allocator* allocator, size_t size, unsigned align);
void temp_allocator_dealloc(Allocator* allocator, void* ptr);
void temp_allocator_dealloc_all(Allocator* allocator);

#define create_temp_allocator() {temp_allocator_alloc, temp_allocator_dealloc, temp_allocator_dealloc_all}

void heap_allocator_check_clean(Allocator* allocator);
void* heap_allocator_alloc(Allocator* allocator, size_t size, unsigned align);
void heap_allocator_dealloc(Allocator* allocator, void* ptr);

#define create_heap_allocator() {heap_allocator_alloc, heap_allocator_dealloc, heap_allocator_check_clean};

void* permanent_allocator_alloc(Allocator* allocator, size_t size, unsigned align);
void permanent_allocator_dealloc(Allocator* allocator, void* ptr);

#define create_permanent_allocator() {permanent_allocator_alloc, permanent_allocator_dealloc, nullptr};
