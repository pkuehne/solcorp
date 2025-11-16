#pragma once
#include <cstdlib>
#include <iostream>

#if defined(_MSC_VER)
#define DEBUG_BREAK() __debugbreak()
#elif defined(__GNUC__)
#define DEBUG_BREAK() __builtin_trap()
#else
#define DEBUG_BREAK() ((void)0)
#endif

#define ASSERT(expr, msg)                                                      \
  do {                                                                         \
    if (!(expr)) {                                                             \
      std::cerr << "Assertion failed:\n"                                       \
                << "  Expr: " << #expr << "\n"                                 \
                << "  Msg:  " << msg << "\n"                                   \
                << "  File: " << __FILE__ << ":" << __LINE__ << "\n";          \
      std::cerr.flush();                                                       \
      DEBUG_BREAK();                                                           \
      std::abort();                                                            \
    }                                                                          \
  } while (0)
