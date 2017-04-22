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

#include "assert.h"

#define ENUM_UTILS(ENUM_NAME)  \
	inline bool FIsValid(ENUM_NAME e) { return (e >= ENUM_NAME##_Min) & (e < ENUM_NAME##_Max); } \
	inline ENUM_NAME VerifyValidElement(ENUM_NAME e) { if (FVERIFY(FIsValid(e), "array access with bad " #ENUM_NAME)) return e; return (ENUM_NAME)0; }

#define MAX_MIN_NIL(ENUM_NAME) ENUM_NAME##_Max, ENUM_NAME##_Min = 0, ENUM_NAME##_Nil = -1 \
	}; ENUM_UTILS(ENUM_NAME) \
	enum ENUM_NAME##_Stub {
