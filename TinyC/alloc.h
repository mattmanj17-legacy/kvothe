/* Copyright (C) 2015 Evan Christensen
|
| Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
| documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
| rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
| persons to whom the Software is furnished to do so, subject to the following conditions:
|
| The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
| Software.
|
| THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
| WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
| COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
| OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */

#pragma once

#include "enum.h"
#include "stb_malloc.h"
#include "macromisc.h"

// Allocator
enum BK // block kind
{
	MAX_MIN_NIL(BK)
};

#define ALLOC(numBytes, alignment) 			AllocImpl(numBytes, alignment, __FILE__, __LINE__)
#define ALLOC_BK(numBytes, alignment, bk) 	AllocImpl(numBytes, alignment, __FILE__, __LINE__, bk)
#define ALLOC_TYPE(TYPE_NAME) 				AllocImpl(sizeof(TYPE_NAME), ALIGN_OF(TYPE_NAME), __FILE__, __LINE__)
#define ALLOC_TYPE_ARRAY(TYPE_NAME, C_MAX) 	AllocImpl(sizeof(TYPE_NAME) * C_MAX, ALIGN_OF(TYPE_NAME), __FILE__, __LINE__)
#define FREE(P) 							FreeImpl(P, __FILE__, __LINE__)
#define NEW(PALLOC, TYPE_NAME)				new ( (PALLOC)->AllocImpl(sizeof(TYPE_NAME), ALIGN_OF(TYPE_NAME), __FILE__, __LINE__))
#define DELETE(P) 							DeleteImpl(P, __FILE__, __LINE__)

void * STBM_CALLBACK SystemAlloc(void * pUserContext, size_t cBRequested, size_t * pCbProvided);
void STBM_CALLBACK SystemFree(void * pUserContext, void *p);

// BB (matthewd) need to impliment allocation tracking 

class CAlloc // tag = alloc
{
public:
				CAlloc();
				CAlloc(void * pBuffer, size_t cB);
				~CAlloc();

	void		Initialize(void * pB, size_t cB);
	void		Shutdown();
	static void	StaticShutdown();

	void *		AllocImpl(size_t cB, size_t cBAlign, const char* pChzFile, int cLine, BK bk = BK_Nil);
	void		FreeImpl(void * pV, const char * pChzFile, int cLine);
	
	template <typename T> 
	void		DeleteImpl(T * p, const char * pChzFile, int cLine);

private:
};


