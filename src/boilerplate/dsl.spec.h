// This is the "minimalistic" C preprocessor file to turn `.dsl` files into their respective "JSON-IR-generating" code.
//
// Recommended run syntax: `g++ -E dsl.inl.h 2>/dev/null | grep -v '^#' | grep -v '^$'`,
// but replace `dsl.inl.h` by the "source" file that first `#include`-s this `dsl.inl.h`.
//
// If you have `clang-format` installed, a good example is:
// g++ -E code.dsl.h 2>/dev/null | grep -v '^#' | grep -v '^$' | clang-format

// NOTE(dkorolev): See the `README.md` in this directory for the "life of the Maroon test case" flow.

#pragma once

#define HLP_EMPTY_HLP_CF_TYPE_EXTRACT
#define HLP_CF_TYPE_PASTE(x, ...) x##__VA_ARGS__
#define HLP_CF_TYPE_PASTE2(x, ...) HLP_CF_TYPE_PASTE(x, __VA_ARGS__)
#define HLP_CF_TYPE_EXTRACT(...) HLP_CF_TYPE_EXTRACT __VA_ARGS__
#define NOPARENS(...) HLP_CF_TYPE_PASTE2(HLP_EMPTY_, HLP_CF_TYPE_EXTRACT __VA_ARGS__)

#define MAROON_SOURCE(s) ctx.out.src = s;
#define MAROON(name) RegisterMaroon(ctx, #name, __LINE__) << [&]()
#define TYPE(name) RegisterType(ctx, #name, __LINE__) << [&]()
#define FIELD(name, type) RegisterField(ctx, #name, #type)
#define FIBER(name) RegisterFiber(ctx, #name, __LINE__) << [&]()
#define FN(name) RegisterFn(ctx, #name, __LINE__) << [&]()
#define STMT(stmt) RegisterStmt(ctx, __LINE__, #stmt);
#define BLOCK RegisterBlock(ctx, __LINE__) << [&]()

// NOTE(dkorolev): Requires extra parentheses around (yes) and (no) in user code. Sigh.
#define IF(cond, yes, no) RegisterIf(ctx, #cond, [&]() { NOPARENS(yes); }, [&]() { NOPARENS(no); }, __LINE__)

#define VAR(name, type, init) RegisterVar(ctx, #name, VarTypes::type, #init, __LINE__);

// NOTE(dkorolev): We will need to make sure the `ARG`-s are only defined at the very top!
// NOTE(dkorolev): Although this is probably unnecessary, since once we have the proper DSL, life will get better.
#define ARG(name, type) RegisterArg(ctx, #name, VarTypes::type, __LINE__);

#define TEST_FIBER(maroon_name, maroon_fiber, ...) \
  {                                                \
    MaroonTestCaseRunFiber t;                      \
    t.line = __LINE__;                             \
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
    t.line = __LINE__;                                          \
    t.maroon = #maroon_name;                                    \
    t.fiber = #maroon_fiber;                                    \
    t.error = err;                                              \
    ctx.out.tests.push_back(std::move(t));                      \
  }
