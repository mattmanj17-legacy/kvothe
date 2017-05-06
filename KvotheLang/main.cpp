#include "types.h"
#include "alloc.h"

// heap size = 1 MB

#define CBYTEHEAP	1024 * 1024

int main()
{
	CAllocHeap halloc(CBYTEHEAP);
	CAllocHeap::SetPHallocDefault(&halloc);
}