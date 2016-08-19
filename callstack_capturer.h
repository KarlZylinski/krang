#pragma once

struct CapturedCallstack
{
    unsigned char num_frames;
    void* frames[64];
    void* ptr;
    bool used;
};

CapturedCallstack callstack_capture(unsigned frames_to_skip, void* p);
void callstack_destroy(CapturedCallstack* c);
void callstack_print(const char* caption, const CapturedCallstack& captured_callstack);
