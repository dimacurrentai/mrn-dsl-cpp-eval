// This is the "minimalistic" C preprocessor file to turn `.dsl` files into their respective "JSON-IR-generating" code.
//
// Recommended run syntax: `g++ -E dsl.inl.h 2>/dev/null | grep -v '^#' | grep -v '^$'`,
// but replace `dsl.inl.h` by the "source" file that first `#include`-s this `dsl.inl.h`.
//
// If you have `clang-format` installed, a good example is:
// g++ -E code.dsl.h 2>/dev/null | grep -v '^#' | grep -v '^$' | clang-format

// TODO(dkorolev): See the `README.md` in this directory for the "life of the Maroon test case" flow.

#pragma once

#define HLP_EMPTY_HLP_CF_TYPE_EXTRACT
#define HLP_CF_TYPE_PASTE(x, ...) x##__VA_ARGS__
#define HLP_CF_TYPE_PASTE2(x, ...) HLP_CF_TYPE_PASTE(x, __VA_ARGS__)
#define HLP_CF_TYPE_EXTRACT(...) HLP_CF_TYPE_EXTRACT __VA_ARGS__
#define NOPARENS(...) HLP_CF_TYPE_PASTE2(HLP_EMPTY_, HLP_CF_TYPE_EXTRACT __VA_ARGS__)

#define MAROON_SOURCE(s) ctx.out.src = s;
#define MAROON(name) RegisterMaroon(ctx, #name) << [&]()
#define FIBER(name) RegisterFiber(ctx, #name) << [&]()
#define FN(name) RegisterFn(ctx, #name) << [&]()
#define STMT(stmt) RegisterStmt(ctx, #stmt);
#define BLOCK RegisterBlock(ctx) << [&]()

// NOTE(dkorolev): Requires extra parentheses around (yes) and (no) in user code. Sigh.
#define IF(cond, yes, no) RegisterIf(ctx, #cond, [&]() { NOPARENS(yes) }, [&]() { NOPARENS(no) })

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
