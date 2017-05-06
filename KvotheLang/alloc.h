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

#define ALLOC(cByte, cByteAlign) 			(CAllocHeap::PHallocDefault())->AllocImpl(cByte, cByteAlign, __FILE__, __LINE__)
#define FREE(p) 							(CAllocHeap::PHallocDefault())->FreeImpl(p, __FILE__, __LINE__)
#define NEW(T, ...)							new ((CAllocHeap::PHallocDefault())->AllocImpl(sizeof(T), ALIGN_OF(T), __FILE__, __LINE__)) T(__VA_ARGS__)
#define DELETE(p) 							(CAllocHeap::PHallocDefault())->DeleteImpl(p, __FILE__, __LINE__)

// BB (matthewd) need to impliment allocation tracking 

// BB (matthewd) need to impliment block kinds...

class CAllocHeap // tag = halloc
{
public:
					CAllocHeap(size_t cByte);
					~CAllocHeap();

	void *			AllocImpl(size_t cByte, size_t cByteAlign, const char* pChzFile, int nLine);
	void			FreeImpl(void * p, const char * pChzFile, int nLine);
	
	template <typename T> 
	void			DeleteImpl(T * p, const char * pChzFile, int nLine);

	static void		SetPHallocDefault(CAllocHeap* pHallocDefault)
						{ s_pHallocDefault = pHallocDefault; }
	static CAllocHeap*	PHallocDefault()
						{ return s_pHallocDefault; }

private:
	stbm_heap*	m_pStbheap;
	static CAllocHeap* s_pHallocDefault;
	u8 * m_aByte;
};


