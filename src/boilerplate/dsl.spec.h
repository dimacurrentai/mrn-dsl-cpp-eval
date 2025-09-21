// This is the "minimalistic" C preprocessor file to turn `.dsl` files into their respective "JSON-IR-generating" code.
//
// Recommended run syntax: `g++ -E dsl.inl.h 2>/dev/null | grep -v '^#' | grep -v '^$'`,
// but replace `dsl.inl.h` by the "source" file that first `#include`-s this `dsl.inl.h`.
//
// If you have `clang-format` installed, a good example is:
// g++ -E code.dsl.h 2>/dev/null | grep -v '^#' | grep -v '^$' | clang-format

// TODO(dkorolev): See the `README.md` in this directory for the "life of the Maroon test case" flow.

#pragma once

#define MAROON(name) RegisterMaroon(ctx, #name) << [&]()
#define FIBER(name) RegisterFiber(ctx, #name) << [&]()
#define FN(name) RegisterFn(ctx, #name) << [&]()
#define STMT(stmt) RegisterStmt(ctx, #stmt);

#define TEST_SIMPLE_RUN(maroon_name, ...)           \
  {                                                 \
    MaroonTestCaseSimple t;                         \
    t.maroon = #maroon_name;                        \
    struct {                                        \
      uint64_t ts;                                  \
      std::string msg;                              \
    } v[] = __VA_ARGS__;                            \
    for (auto const& [ts, msg] : v) {               \
      MaroonDebugStatement tmp;                     \
      tmp.ts = ts;                                  \
      tmp.msg = msg;                                \
      t.debug_statements.push_back(std::move(tmp)); \
    }                                               \
    ctx.out.tests.push_back(std::move(t));          \
  }

#define TEST_SHOULD_THROW(maroon_name, err) \
  {                                         \
    MaroonTestCaseShouldThrow t;            \
    t.maroon = #maroon_name;                \
    t.error = err;                          \
    ctx.out.tests.push_back(std::move(t));  \
  }
