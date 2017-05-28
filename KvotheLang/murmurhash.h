#pragma once

#include "types.h"

void MurmurHash3_x86_32  ( const void * key, int len, void * out, u32 seed = 0 );

void MurmurHash3_x86_128 ( const void * key, int len, void * out, u32 seed = 0 );

void MurmurHash3_x64_128 ( const void * key, int len, void * out, u32 seed = 0 );
