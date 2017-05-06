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

#include "alloc.h"

// resizable array (aka std::vector)

template <typename T>
class CDynAry : public CAry<T> //tag = dary
{
public:
				CDynAry();
				~CDynAry();
				CDynAry(const CAry&) = delete;
				CDynAry & operator=(const CAry&) = delete;

	void		EnsureCapacity(size_t cMax);

	const T &	operator[](size_t i) const;
	T &			operator[](size_t i);

	T *			A()
					{ return m_a; }
	const T*	A() const
					{ return m_a; }

	size_t		C() const
					{ return m_c; }
	size_t		CMax() const
					{ return m_cMax; }
	bool		FIsEmpty() const
					{ return m_c == 0; }

	size_t		IFind(const T * p) const;

	T *			begin()
					{ return m_a; }
	T *			end()
					{ return m_a + m_c; }

	void		Clear();

	void		Append(const T & t);
	T *			AppendNew();

private:
	T * m_a;
	size_t m_c;
	size_t m_cMax;
};