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

#include "types.h"

void AssertImpl(const char* pChzFile, u32 line, const char* pChzCondition, const char* pChzMessage = 0, ...);

#define DEBUG_BREAK() __debugbreak()

#define VERIFY(predicate, ...) do { if (!(predicate)) { AssertImpl(__FILE__, __LINE__, #predicate, __VA_ARGS__); DEBUG_BREAK(); } } while(0)

#define ASSERT(predicate, ...) VERIFY(predicate, __VA_ARGS__)

#define FVERIFY(predicate, ...) ( ( (predicate) ? true : ( AssertImpl(__FILE__, __LINE__, #predicate, __VA_ARGS__), DEBUG_BREAK(), false ) ) )

#define FASSERT(predicate, ...) FVERIFY(predicate, __VA_ARGS__)


