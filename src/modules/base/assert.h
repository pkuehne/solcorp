#pragma once
#include <cstdlib>
#include <iostream> // IWYU pragma: keep

#ifndef SC_ASSERT_BREAK_ON_FAIL
#define SC_ASSERT_BREAK_ON_FAIL 0
#endif

#if defined(_MSC_VER)
#define DEBUG_BREAK() __debugbreak()
#elif defined(__GNUC__)
#define DEBUG_BREAK() __builtin_trap()
#else
#define DEBUG_BREAK() ((void)0)
#endif

#define SC_ASSERT(expr, msg)                                                   \
  do {                                                                         \
    if (!(expr)) {                                                             \
      std::cerr << "Assertion failed:\n"                                       \
                << "  Expr: " << #expr << "\n"                                 \
                << "  Msg:  " << (msg) << "\n"                                 \
                << "  File: " << __FILE__ << ":" << __LINE__ << "\n";          \
      std::cerr.flush();                                                       \
      if (SC_ASSERT_BREAK_ON_FAIL) {                                           \
        DEBUG_BREAK();                                                         \
      }                                                                        \
      std::exit(EXIT_FAILURE);                                                 \
    }                                                                          \
  } while (0)
