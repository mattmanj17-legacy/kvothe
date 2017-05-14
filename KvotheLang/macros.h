#pragma once
#include <assert.h>

// DIM macro
// http://stackoverflow.com/a/6256085

#define DIM(arr) ( \
   0 * sizeof(reinterpret_cast<const ::Bad_arg_to_DIM*>(arr)) + \
   0 * sizeof(::Bad_arg_to_DIM::check_type((arr), &(arr))) + \
   sizeof(arr) / sizeof((arr)[0]) )

struct Bad_arg_to_DIM {
   class Is_pointer; // incomplete
   class Is_array {};
   template <typename T>
   static Is_pointer check_type(const T*, const T* const*);
   static Is_array check_type(const void*, const void*);
};

// CASSERT

#define CASSERT(predicate) static_assert(predicate, #predicate)
