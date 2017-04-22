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

template <typename T>
class CAry // tag = ary
{
public:

	class CIterator // tag = iter
	{
	public:
			CIterator(CAry<T> * pAry);

		T * Next();

	private:
		T * m_pCur;
		T * m_pEnd;
	};

				CAry(BK bk = BK_Nil);
				CAry(T * a, s32 c, s32 cMax, BK bk = BK_Nil);
				CAry(const CAry&) = delete;
				CAry & operator=(const CAry&) = delete;

	const T &	operator[](size_t i) const;
	T &			operator[](size_t i);

	size_t		C() const;

	// for range based for
	// BB (matthewd) return a CIterator?

	T *			begin();
	T *			end();

private:
};

// resizable array (aka std::vector)
template <typename T>
class CDynAry : public CAry<T> //tag = dary
{
public:
			CDynAry(CAlloc * pAlloc, BK bk, s32 cMaxStarting = 16);
			CDynAry(BK bk = BK_Nil);
			~CDynAry();
			CDynAry(const CDynAry & rhs);
			CDynAry<T> & operator= (const CDynAry & rhs);

	void	Append(const T t);
	void	Append(const Type * pTArray, size_t cT);
	void	AppendFill(size_t c, const Type t);
	T *		AppendNew()
};

// fixed sized array container template
template <typename T, int C_MAX>
class CFixAry : public CAry<T> // tag = fary
{
};