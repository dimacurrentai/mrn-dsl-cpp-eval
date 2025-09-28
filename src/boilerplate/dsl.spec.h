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
#define BLOCK(blk)                        \
  {                                       \
    RegisterBlock const block_scope(ctx); \
    blk;                                  \
  }
#define VAR(name, type, init) RegisterVar(ctx, #name, VarTypes::type, #init);

#define TEST_FIBER(maroon_name, maroon_fiber, ...) \
  {                                                \
    MaroonTestCaseRunFiber t;                      \
    t.maroon = #maroon_name;                       \
    t.fiber = #maroon_fiber;                       \
    std::string v[] = __VA_ARGS__;                 \
    for (auto const& msg : v) {                    \
      t.golden_output.push_back(msg);              \
    }                                              \
    ctx.out.tests.push_back(std::move(t));         \
  }

#define TEST_FIBER_SHOULD_THROW(maroon_name, maroon_fiber, err) \
  {                                                             \
    MaroonTestCaseFiberShouldThrow t;                           \
    t.maroon = #maroon_name;                                    \
    t.fiber = #maroon_fiber;                                    \
    t.error = err;                                              \
    ctx.out.tests.push_back(std::move(t));                      \
  }
