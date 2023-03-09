#pragma once
#include <assert.h>
#include <comdef.h>
#include <sstream>

template< typename... Args >
inline void print_assertion(Args&&... args) {
    std::stringstream ss;
    ss.precision(10);
    ss << std::endl;
    (ss << ... << args) << std::endl;
    OutputDebugString(ss.str().c_str());
    abort();
}

#ifndef ASSERT_FORMAT
#define ASSERT_FORMAT(EXPRESSION, ... ) ((EXPRESSION) ? (void)0 : print_assertion(\
        "Error: ", \
        #EXPRESSION, \
        " in File: ", \
        __FILE__, \
        " in Line: ", \
        __LINE__, \
        " \n",\
        __VA_ARGS__))
#endif


#ifndef HRASSERT
#define HRASSERT(x)                                              \
{                                                                     \
    HRESULT hr__ = (x);                                               \
    _com_error comError{ hr__ };                                      \
    ASSERT_FORMAT(!FAILED(hr__), comError.ErrorMessage()); \
}
#endif