#pragma once

template <typename T, int size>
class CCircleBuffer // tag = cbuf
{
public:

	CCircleBuffer();
	int CLength();
	void Push(T t);
	T& operator[](int i)
		{ return At(i); }

private:
	T& At(int i);

	T m_a[c_size];
	int m_iBegin;
};
