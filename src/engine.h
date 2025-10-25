// NOTE(dkorolev): This is the somewhat ugly piece of code to "execute" the "post-DSL" boilerplate.

#pragma once

#include <vector>
#include <functional>
#include <iostream>
#include <sstream>

#include "../current/bricks/exception.h"

// TODO(dkorolev): This will go away soon.
CURRENT_STRUCT(MAROON_TYPE_U64) {
  CURRENT_FIELD(value, uint64_t);
  CURRENT_CONSTRUCTOR(MAROON_TYPE_U64)(uint64_t v = 0) : value(v) {}
};

CURRENT_STRUCT(MAROON_TYPE_BOOL) {
  CURRENT_FIELD(value, bool);
  CURRENT_CONSTRUCTOR(MAROON_TYPE_BOOL)(bool v = false) : value(v) {}
};

#define MAROON_BASE_TYPES_CSV MAROON_TYPE_U64, MAROON_TYPE_BOOL

// TODO(dkorolev): This is kinda ugly, although seemingly necessary — need to reconcile for the future.

#define DEFINE_BINARY_OP(type, op1, op2)                                                               \
  inline type operator op1(type const& lhs, type const& rhs) { return type(lhs.value op1 rhs.value); } \
  template <typename IMMEDIATE>                                                                        \
  inline type operator op1(type const& lhs, IMMEDIATE rhs) {                                           \
    return type(lhs.value op1 rhs);                                                                    \
  }                                                                                                    \
  inline type& operator op2(type & lhs, type const& rhs) {                                             \
    lhs.value op2 rhs.value;                                                                           \
    return lhs;                                                                                        \
  }                                                                                                    \
  template <typename IMMEDIATE>                                                                        \
  inline type& operator op2(type & lhs, IMMEDIATE rhs) {                                               \
    lhs.value op2 rhs;                                                                                 \
    return lhs;                                                                                        \
  }

#define DEFINE_BOOLEAN_OP(type, op)                                                            \
  inline bool operator op(type const& lhs, type const& rhs) { return lhs.value op rhs.value; } \
  template <typename IMMEDIATE>                                                                \
  inline bool operator op(type const& lhs, IMMEDIATE rhs) {                                    \
    return lhs.value op rhs;                                                                   \
  }

DEFINE_BINARY_OP(MAROON_TYPE_U64, +, +=)
DEFINE_BINARY_OP(MAROON_TYPE_U64, -, -=)
DEFINE_BINARY_OP(MAROON_TYPE_U64, *, *=)
DEFINE_BOOLEAN_OP(MAROON_TYPE_U64, ==)
DEFINE_BOOLEAN_OP(MAROON_TYPE_U64, !=)
DEFINE_BOOLEAN_OP(MAROON_TYPE_U64, <)
DEFINE_BOOLEAN_OP(MAROON_TYPE_U64, <=)
DEFINE_BOOLEAN_OP(MAROON_TYPE_U64, >)
DEFINE_BOOLEAN_OP(MAROON_TYPE_U64, >=)

class MaroonDefinition {
 public:
  virtual char const* const maroon_name() const = 0;

 protected:
  ~MaroonDefinition() = default;
};

// TODO(dkorolev): If we agree it's uint32_t, need to make sure the future compiler checks the size of the program.
enum class MaroonStateIndex : uint32_t;
enum class MaroonVarIndex : uint32_t;

struct ImplException : current::Exception {
  using current::Exception::Exception;
};

template <class, class, class...>
struct MaroonPackArgsImpl;

template <class T_VARS_TYPELIST, class OUT_TYPE, class... OUT_TYPES, typename IN_TYPE, typename... IN_TYPES>
struct MaroonPackArgsImpl<T_VARS_TYPELIST, std::tuple<OUT_TYPE, OUT_TYPES...>, IN_TYPE, IN_TYPES...> final {
  static void DoIt(std::vector<T_VARS_TYPELIST>& res, IN_TYPE&& x, IN_TYPES&&... xs) {
    OUT_TYPE y(std::forward<IN_TYPE>(x));
    res.push_back(std::move(y));
    MaroonPackArgsImpl<T_VARS_TYPELIST, std::tuple<OUT_TYPES...>, IN_TYPES...>::DoIt(res,
                                                                                     std::forward<IN_TYPES>(xs)...);
  }
};

template <class T_VARS_TYPELIST>
struct MaroonPackArgsImpl<T_VARS_TYPELIST, std::tuple<>> {
  static void DoIt(std::vector<T_VARS_TYPELIST>&) {}
};

template <class T_VARS_TYPELIST, class OUT_TYPELIST, typename... IN_TYPES>
std::vector<T_VARS_TYPELIST> pack_args(IN_TYPES&&... args) {
  std::vector<T_VARS_TYPELIST> res;
  MaroonPackArgsImpl<T_VARS_TYPELIST, OUT_TYPELIST, IN_TYPES...>::DoIt(res, std::forward<IN_TYPES>(args)...);
  return res;
}

enum TmpNextStatus { None = 0, Next, Branch, Call, Return };

template <class T_VARS_TYPELIST>
struct ImplResultCollector final {
  MaroonStateIndex next_idx_;
  TmpNextStatus status_ = TmpNextStatus::None;

  MaroonStateIndex call_idx_;
  std::string call_f_;
  MaroonVarIndex call_retval_var_idx_;

  std::vector<T_VARS_TYPELIST> call_args_;

  bool has_retval_;
  T_VARS_TYPELIST retval_;

  void next() {
    if (status_ != TmpNextStatus::None) {
      CURRENT_THROW(ImplException("TODO(dkorolev): FIXME: Attempted `NEXT()` in the wrong place."));
    }
    status_ = TmpNextStatus::Next;
  }
  void branch(MaroonStateIndex idx) {
    if (status_ != TmpNextStatus::None) {
      CURRENT_THROW(ImplException("TODO(dkorolev): FIXME: Attempted to `IF()` in the wrong place."));
    }
    status_ = TmpNextStatus::Branch;
    next_idx_ = idx;
  }

  void call_ignore_return(size_t number_of_args,
                          MaroonStateIndex idx,
                          std::string f,
                          std::vector<T_VARS_TYPELIST> args) {
    if (status_ != TmpNextStatus::None) {
      CURRENT_THROW(ImplException("TODO(dkorolev): FIXME: Attempted `CALL()` in the wrong place."));
    }
    if (args.size() != number_of_args) {
      // TODO(dkorolev): The error message should make sense. Including `file:line` perhaps.
      CURRENT_THROW(ImplException("WRONG NUMBER OF ARGS"));
    }
    status_ = TmpNextStatus::Call;
    call_idx_ = idx;
    call_f_ = std::move(f);
    call_retval_var_idx_ = static_cast<MaroonVarIndex>(-1);
    call_args_ = std::move(args);
  }

  void call_capture_return(
      MaroonVarIndex v, size_t number_of_args, MaroonStateIndex idx, std::string f, std::vector<T_VARS_TYPELIST> args) {
    if (status_ != TmpNextStatus::None) {
      CURRENT_THROW(ImplException("TODO(dkorolev): FIXME: Attempted `CALL()` in the wrong place."));
    }
    if (args.size() != number_of_args) {
      // TODO(dkorolev): The error message should make sense. Including `file:line` perhaps.
      CURRENT_THROW(ImplException("WRONG NUMBER OF ARGS"));
    }
    status_ = TmpNextStatus::Call;
    call_idx_ = idx;
    call_f_ = std::move(f);
    call_retval_var_idx_ = v;
    call_args_ = std::move(args);
  }

  template <typename T_UNUSED_FUNCTION_RETURN_TYPE>
  void ret() {
    if (status_ != TmpNextStatus::None) {
      CURRENT_THROW(ImplException("TODO(dkorolev): FIXME: Attempted `RETURN()` in the wrong place."));
    }
    status_ = TmpNextStatus::Return;
    has_retval_ = false;
  }

  template <typename T_FUNCTION_RETURN_TYPE, typename T_ARG>
  void ret(T_ARG&& val) {
    if (status_ != TmpNextStatus::None) {
      CURRENT_THROW(ImplException("TODO(dkorolev): FIXME: Attempted `RETURN()` in the wrong place."));
    }
    status_ = TmpNextStatus::Return;
    has_retval_ = true;
    T_FUNCTION_RETURN_TYPE tmp = T_FUNCTION_RETURN_TYPE(std::forward<T_ARG>(val));
    retval_ = T_VARS_TYPELIST(std::move(tmp));
  }

  TmpNextStatus status() const {
    if (status_ == TmpNextStatus::None) {
      CURRENT_THROW(ImplException("`AWAIT` or `RETURN` condition missing in an `STMT`."));
    }
    return status_;
  }
};

template <class T_VARS_TYPELIST>
struct ImplVar final {
  std::string name;
  T_VARS_TYPELIST value;
};

template <class T_VARS_TYPELIST>
struct ImplCallStackEntry final {
  MaroonStateIndex current_idx_;

  std::string f_;
  MaroonVarIndex call_retval_var_idx_;
  std::vector<ImplVar<T_VARS_TYPELIST>> vars_;

  size_t args_used_ = 0u;
  std::vector<T_VARS_TYPELIST> args_;

  ImplCallStackEntry() = delete;
  explicit ImplCallStackEntry(MaroonStateIndex idx,
                              std::string f = "",
                              MaroonVarIndex call_retval_var_idx = static_cast<MaroonVarIndex>(-1))
      : current_idx_(idx), f_(std::move(f)), call_retval_var_idx_(call_retval_var_idx) {}

  ImplCallStackEntry(ImplCallStackEntry const&) = default;
  ImplCallStackEntry& operator=(ImplCallStackEntry const&) = default;
};

template <typename T>
struct MaroonFormatValueHelperImpl final {
  static void DoIt(std::ostream& os, T const& v) { os << JSON(v); }
};

template <>
struct MaroonFormatValueHelperImpl<MAROON_TYPE_U64> final {
  static void DoIt(std::ostream& os, MAROON_TYPE_U64 const& v) { os << v.value; }
};

template <>
struct MaroonFormatValueHelperImpl<MAROON_TYPE_BOOL> final {
  static void DoIt(std::ostream& os, MAROON_TYPE_BOOL const& v) { os << std::boolalpha << v.value; }
};

struct MaroonFormatValueHelper final {
  std::ostream& os;
  MaroonFormatValueHelper(std::ostream& os) : os(os) {}

  template <class T>
  void operator()(T const& v) const {
    MaroonFormatValueHelperImpl<T>::DoIt(os, v);
  }
};

template <typename T>
void MaroonFormatValue(std::ostream& os, T const& var) {
  MaroonFormatValueHelper helper(os);
  var.Call(helper);
}

template <class T_VARS_TYPELIST>
struct ImplEnv final {
  std::ostream& os_;

  std::vector<ImplCallStackEntry<T_VARS_TYPELIST>> call_stack_;

  explicit ImplEnv(std::ostream& os) : os_(os) {}

  template <typename T>
  void debug(T&& v, char const* const file, int line) {
    std::ostringstream oss;
    oss << std::forward<T>(v);
    std::string const s = oss.str();
    std::cerr << "Impl DEBUG: " << s << " @ " << file << ':' << line << std::endl;
    // TODO(dkorolev): Tick index / time.
    os_ << s << std::endl;
  }

  template <typename T>
  void debug_expr(char const* const expr, T&& v, char const* const file, int line) {
    std::ostringstream oss;
    oss << expr << '=';
    (MaroonFormatValueHelper(oss))(std::forward<T>(v));
    std::string const s = oss.str();
    std::cerr << "Impl DEBUG: " << s << " @ " << file << ':' << line << std::endl;
    // TODO(dkorolev): Tick index / time.
    os_ << s << std::endl;
  }

  void debug_dump_vars(char const* const file, int line) {
    std::ostringstream oss;
    do_debug_dump_vars(oss, call_stack_.back().vars_, file, line);
    std::string const s = oss.str();
    std::cerr << "Impl VARS: " << s << " @ " << file << ':' << line << std::endl;
    // TODO(dkorolev): Tick index / time.
    os_ << s << std::endl;
  }

  void do_debug_dump_vars(std::ostringstream& oss,
                          std::vector<ImplVar<T_VARS_TYPELIST>> const& vars,
                          char const* const file,
                          int line) {
    oss << '[';
    bool first = true;
    for (auto const& v : vars) {
      if (first) {
        first = false;
      } else {
        oss << ',';
      }
      oss << v.name << ':';
      MaroonFormatValue(oss, v.value);
    }
    oss << ']';
  }

  void debug_dump_stack(char const* const file, int line) {
    std::ostringstream oss;
    oss << '<';
    bool first = true;
    for (auto const& v : call_stack_) {
      if (first) {
        first = false;
      } else {
        oss << ',';
      }
      if (!v.f_.empty()) {
        oss << v.f_ << '@';
      }
      do_debug_dump_vars(oss, v.vars_, file, line);
    }
    oss << '>';
    std::string const s = oss.str();
    std::cerr << "Impl STACK: " << s << " @ " << file << ':' << line << std::endl;
    os_ << s << std::endl;
  }

  template <typename T_VAR>
  void DeclareVar(size_t idx, std::string name, uint64_t init) {
    if (idx != call_stack_.back().vars_.size()) {
      std::cerr << "Internal invariant error: corrupted stack." << std::endl;
      std::exit(1);
    }
    ImplVar<T_VARS_TYPELIST> var;
    var.name = std::move(name);
    T_VAR val = T_VAR(init);
    var.value = T_VARS_TYPELIST(std::move(val));
    call_stack_.back().vars_.push_back(std::move(var));
  }

  template <typename T_VAR>
  void DeclareFunctionArg(size_t idx, std::string name) {
    if (idx != call_stack_.back().vars_.size()) {
      std::cerr << "Internal invariant error: corrupted stack." << std::endl;
      std::exit(1);
    }
    if (call_stack_.back().args_used_ >= call_stack_.back().args_.size()) {
      std::cerr << "Internal invariant error: not enough args, should never happen." << std::endl;
      std::exit(1);
    }
    ImplVar<T_VARS_TYPELIST> var;
    var.name = std::move(name);
    // TODO(dkorolev): Check that we're not out of `args_used_`!
    var.value = std::move(call_stack_.back().args_[call_stack_.back().args_used_++]);
    call_stack_.back().vars_.push_back(std::move(var));
  }

  template <class T_VAR>
  T_VAR& AccessVar(size_t idx, char const* const name) {
    if (idx >= call_stack_.back().vars_.size()) {
      std::cerr << "Internal invariant error: var out of stack." << std::endl;
      std::exit(1);
    }
    if (call_stack_.back().vars_[idx].name != name) {
      std::cerr << "Internal invariant error: corrupted stack, at index " << idx << " expecting var " << name
                << ", have var " << call_stack_.back().vars_[idx].name << std::endl;
      std::exit(1);
    }
    auto& v = call_stack_.back().vars_[idx].value;
    if (Exists<T_VAR>(v)) {
      return Value<T_VAR>(v);
    } else {
      // TODO(dkorolev): This error message can be more verbose with `TypeName`-s everywhere.
      std::cerr << "Internal invariant error: var `" << name << "` at index " << idx << " if of the wrong type."
                << std::endl;
      std::exit(1);
    }
  }
};

template <class T_VARS_TYPELIST>
using step_function_t = void (*)(ImplEnv<T_VARS_TYPELIST>& env, ImplResultCollector<T_VARS_TYPELIST>& result);

template <class T_VARS_TYPELIST>
using vars_function_t = void (*)(ImplEnv<T_VARS_TYPELIST>& env);

template <class T_VARS_TYPELIST>
struct MaroonStep final {
  step_function_t<T_VARS_TYPELIST> code;
  size_t num_vars_available_before_step;
  size_t num_vars_declared_for_step;
  vars_function_t<T_VARS_TYPELIST> new_vars;
};

#define DEBUG(s) MAROON_env.debug(s, __FILE__, __LINE__)
#define DEBUG_EXPR(s) MAROON_env.debug_expr(#s, s, __FILE__, __LINE__)
#define DEBUG_DUMP_VARS() MAROON_env.debug_dump_vars(__FILE__, __LINE__)
#define DEBUG_DUMP_STACK() MAROON_env.debug_dump_stack(__FILE__, __LINE__)
#define NEXT() MAROON_result.next()

// NOTE(dkorolev): The ugly yet functional way to tell 1-arg vs. 2-args macros.
#define CALL_DISPATCH(_1, _2, _3, NAME, ...) NAME
#define CALL(...) CALL_DISPATCH(__VA_ARGS__, CALL3, CALL2, NONEXISTENT_CALL1)(__VA_ARGS__)

#define CALL2(f, args) \
  MAROON_result.call_ignore_return(NUMBER_OF_ARGS_##f, FN_##f, #f, pack_args<types_t, MAROON_F_ARGS_##f> args)
#define CALL3(v, f, args)            \
  MAROON_result.call_capture_return( \
      MAROON_VAR_INDEX_##v, NUMBER_OF_ARGS_##f, FN_##f, #f, pack_args<types_t, MAROON_F_ARGS_##f> args)

#define RETURN(...) MAROON_result.ret<T_FUNCTION_RETURN_TYPE>(__VA_ARGS__)

template <class T_MAROON, class T_FIBER>
struct MaroonEngine final {
  static_assert(std::is_base_of<MaroonDefinition, T_MAROON>::value, "");
  // TODO(dkorolev): Perhaps add a `static_assert` that this `T_FIBER` is from the right `T_MAROON`.
  using T_VARS_TYPELIST = typename T_MAROON::maroon_namespace_types_t;

  // NOTE(dkorolev): This will not compile if there's no `main` in the `global` fiber.
  static_assert(T_FIBER::kIsFiber, "");

  static_assert(T_FIBER::NUMBER_OF_ARGS_main == 0, "");

  std::pair<std::string, std::string> run() {
    try {
      std::ostringstream oss;
      ImplEnv<T_VARS_TYPELIST> env(oss);

      auto const fiber_steps = T_FIBER::MAROON_steps();

      // TODO(dkorolev): Proper engine =)
      env.call_stack_.push_back(ImplCallStackEntry<T_VARS_TYPELIST>(T_FIBER::FN_main));
      while (!env.call_stack_.empty()) {
        if (static_cast<uint32_t>(env.call_stack_.back().current_idx_) >= T_FIBER::kStepsCount) {
          CURRENT_THROW(ImplException("NEXT() out of bounds."));
        }
        MaroonStep<T_VARS_TYPELIST> const& step =
            fiber_steps[static_cast<uint32_t>(env.call_stack_.back().current_idx_)];

        if (env.call_stack_.back().vars_.size() < step.num_vars_available_before_step) {
          std::cerr << "Internal invariant failed: pre-step vars count mismatch." << std::endl;
          std::exit(1);
        }

        if (env.call_stack_.back().vars_.size() > step.num_vars_available_before_step) {
          // Destruct what is no longer needed.
          env.call_stack_.back().vars_.resize(step.num_vars_available_before_step);
        }

        step.new_vars(env);

        if (env.call_stack_.back().vars_.size() !=
            step.num_vars_available_before_step + step.num_vars_declared_for_step) {
          std::cerr << "Internal invariant failed: intra-step vars count mismatch." << std::endl;
          std::exit(1);
        }

        ImplResultCollector<T_VARS_TYPELIST> result;
        step.code(env, result);

        if (result.status() == TmpNextStatus::Next) {
          env.call_stack_.back().current_idx_ =
              static_cast<MaroonStateIndex>(static_cast<uint32_t>(env.call_stack_.back().current_idx_) + 1);
        } else if (result.status() == TmpNextStatus::Branch) {
          env.call_stack_.back().current_idx_ = static_cast<MaroonStateIndex>(result.next_idx_);
        } else if (result.status() == TmpNextStatus::Call) {
          env.call_stack_.back().current_idx_ =
              static_cast<MaroonStateIndex>(static_cast<uint32_t>(env.call_stack_.back().current_idx_) + 1);
          env.call_stack_.push_back(ImplCallStackEntry<T_VARS_TYPELIST>(
              result.call_idx_, std::move(result.call_f_), result.call_retval_var_idx_));
          env.call_stack_.back().args_ = std::move(result.call_args_);
        } else if (result.status() == TmpNextStatus::Return) {
          auto const retval_var_idx = env.call_stack_.back().call_retval_var_idx_;
          env.call_stack_.pop_back();
          if (result.has_retval_) {
            if (env.call_stack_.empty()) {
              std::cerr << "Internal error: returning from the top-level of the fiber should have no value."
                        << std::endl;
              std::exit(1);
            }
            if (retval_var_idx != static_cast<MaroonVarIndex>(-1)) {
              env.call_stack_.back().vars_[static_cast<size_t>(retval_var_idx)].value = result.retval_;
            }
            // NOTE(dkorolev): Perfectly fine to ignore the returned value!
          } else if (retval_var_idx != static_cast<MaroonVarIndex>(-1)) {
            CURRENT_THROW(ImplException("A return value must have been provided."));
          }
        } else {
          std::cerr << "Internal error: this should never happen." << std::endl;
          std::exit(1);
        }

        // TODO(dkorolev): Clean up the vars here, not up there.
      }

      return {oss.str(), ""};
    } catch (ImplException const& e) {
      return {"", e.OriginalDescription()};
    }
  }
};
