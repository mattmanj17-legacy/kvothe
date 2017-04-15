#include "circlebuffer.h"
#include <exception>

using std::exception;

template <typename T, int size>
CCircleBuffer<T, size>::CCircleBuffer()
: m_iBegin(0)
{
}

template <typename T, int size>
int CCircleBuffer<T, size>::CLength()
{
	return size;
}

template <typename T, int size>
void CCircleBuffer<T, size>::Push(T t)
{
	At(0) = t;
	++m_iBegin;
	m_iBegin %= CLength();
}

template <typename T, int size>
T& CCircleBuffer<T, size>::At(int i)
{
	if (i < 0) { throw exception(); }

	i += m_iBegin;
	i %= CLength();

	return m_a[i];
}