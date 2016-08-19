#pragma once

struct Allocator;
struct LexToken;

ParseScope parse(Allocator* alloc, const LexToken* lex_tokens, size_t num_lex_tokens)