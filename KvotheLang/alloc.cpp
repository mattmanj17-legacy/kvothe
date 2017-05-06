#include "alloc.h"
#include "assert.h"

#include <stdlib.h> // malloc, free

void * STBM_CALLBACK SystemAlloc(void * pUserContext, size_t cByteRequested, size_t * pCByteProvided)
{
	(void)pUserContext;
	
	void * pReturn = malloc(cByteRequested);
	*pCByteProvided = pReturn ? cByteRequested : 0;
	return pReturn;
}

void STBM_CALLBACK SystemFree(void * pUserContext, void *p)
{
	(void)pUserContext;

	free(p);
}

CAllocHeap::CAllocHeap(size_t cByte)
{
	ASSERT(cByte > STBM_HEAP_SIZEOF, "heap is too small");

	m_aByte = new u8[cByte];

	stbm_heap_config config;

	config.system_alloc = SystemAlloc;
	config.system_free = SystemFree;
	config.user_context = nullptr;

	config.minimum_alignment = 8;
	config.align_all_blocks_to_minimum = false;

	config.allocation_mutex = nullptr;
	config.crossthread_free_mutex = nullptr;

	m_pStbheap = stbm_heap_init(m_aByte, cByte, &config);
	ASSERT(m_pStbheap);
}

CAllocHeap::~CAllocHeap()
{
	stbm_heap_free(m_pStbheap);
	delete[] m_aByte;
}

void * CAllocHeap::AllocImpl(size_t cByte, size_t cByteAlign, const char* pChzFile, int nLine)
{
	ASSERT(m_pStbheap, "Trying to allocate from a NULL heap");

	void * p = stbm_alloc_align_fileline(
					nullptr, 
					m_pStbheap, 
					cByte, 
					0, 
					cByteAlign, 
					0, 
					const_cast<char*>(pChzFile), 
					nLine);

	ASSERT(p);
	
	return p;
}

void CAllocHeap::FreeImpl(void * pV, const char * pChzFile, int nLine)
{
	ASSERT(pV, "NULL pointer in CAlloc::FreeImpl");
	ASSERT(m_pStbheap, "Trying to free to a NULL heap");

	size_t cByte = stbm_get_allocation_size(pV);

	stbm_free(nullptr, m_pStbheap, pV);
}

template <typename T> 
void CAllocHeap::DeleteImpl(T * p, const char * pChzFile, int nLine)
{
	p->~T();
	FreeImpl(p, pChzFile, nLine);
}