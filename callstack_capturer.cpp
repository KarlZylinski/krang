#include "callstack_capturer.h"
#include <Windows.h>
#pragma warning(push)
#pragma warning(disable : 4091)
#include <DbgHelp.h>
#pragma warning(pop)

CapturedCallstack callstack_capture(unsigned frames_to_skip, void* p)
{
    const unsigned frames_to_capture = 64;
    DWORD back_trace_hash = 0; 
    CapturedCallstack cc;
    cc.num_frames = (unsigned char)CaptureStackBackTrace(frames_to_skip + 2, frames_to_capture, cc.frames, &back_trace_hash);
    cc.ptr = p;
    cc.used = true;
    return cc;
}

void callstack_destroy(CapturedCallstack* c)
{
    c->used = false;
}

void callstack_print(const char* caption, const CapturedCallstack& captured_callstack)
{
    HANDLE process = GetCurrentProcess();
    SymInitialize(process, NULL, TRUE);
    SYMBOL_INFO* symbol = (SYMBOL_INFO *)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    auto callstack_str = (char*)malloc(symbol->MaxNameLen * 64);
    unsigned callstack_str_size = 0;
    for (unsigned i = 0; i < captured_callstack.num_frames; i++ )
    {
        SymFromAddr(process, (DWORD64)(captured_callstack.frames[i]), 0, symbol);
        memcpy(callstack_str + callstack_str_size, symbol->Name, symbol->NameLen);
        callstack_str[callstack_str_size + symbol->NameLen] = '\n';
        callstack_str_size += symbol->NameLen + 1;
    }
    callstack_str[callstack_str_size] = 0;
    MessageBox(nullptr, callstack_str, caption, MB_ICONERROR);
    free(callstack_str);
    free(symbol);
}
