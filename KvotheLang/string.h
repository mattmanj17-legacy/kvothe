#pragma once

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

#include "alloc.h"

// utf8 - wide char strings

class CString // tag=str
{
public:
	static void		StaticInit(CAlloc * pAlloc);
	static void		StaticShutdown(CAlloc * pAlloc);

	CString();
	CString(const char * pChz);
	CString(const char * pCh, size_t cB);
	CString(const CString & strOther);
	~CString();
	bool			operator==(const CString & strOther) const;
	bool			operator!=(const CString & strOther) const;
	CString &		operator=(const CString & strOther);
	CString &		operator=(const char * pChz);
};