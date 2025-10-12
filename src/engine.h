// NOTE(dkorolev): This is the somewhat ugly piece of code to "execute" the "post-DSL" boilerplate.

#pragma once

#include <vector>
#include <functional>
#include <iostream>
#include <sstream>

#include "../current/bricks/exception.h"

// TODO(dkorolev): If we agree it's uint32_t, need to make sure the future compiler checks the size of the program.
enum class MaroonStateIndex : uint32_t;
enum class MaroonVarIndex : uint32_t;

// TODO(dkorolev): Support different variable types =) and overall the Maroon stack here.
enum class VarValue : uint64_t;

struct ImplException : current::Exception {
  using current::Exception::Exception;
};

template <typename... TS>
inline std::vector<uint64_t> pack_args(TS... args) {
  return {static_cast<uint64_t>(args)...};
}

enum TmpNextStatus { None = 0, Next, Branch, Done, Call, Return };
struct ImplResultCollector final {
  MaroonStateIndex next_idx_;
  TmpNextStatus status_ = TmpNextStatus::None;

  MaroonStateIndex call_idx_;
  std::string call_f_;
  MaroonVarIndex call_retval_var_idx_;
  std::vector<uint64_t> call_args_;

  bool has_retval_;
  VarValue retval_;

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
  void done() {
    if (status_ != TmpNextStatus::None) {
      CURRENT_THROW(ImplException("TODO(dkorolev): FIXME: Attempted `DONE()` in the wrong place."));
    }
    status_ = TmpNextStatus::Done;
  }

  void call_ignore_return(size_t number_of_args, MaroonStateIndex idx, std::string f, std::vector<uint64_t> args) {
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
      MaroonVarIndex v, size_t number_of_args, MaroonStateIndex idx, std::string f, std::vector<uint64_t> args) {
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

  void ret() {
    status_ = TmpNextStatus::Return;
    has_retval_ = false;
  }

  template <typename T>
  void ret(T val) {
    status_ = TmpNextStatus::Return;
    has_retval_ = true;
    retval_ = static_cast<VarValue>(val);
  }

  TmpNextStatus status() const {
    if (status_ == TmpNextStatus::None) {
      CURRENT_THROW(ImplException("`AWAIT` or `RETURN` condition missing in an `STMT`."));
    }
    return status_;
  }
};

struct ImplVar final {
  std::string name;
  VarValue value;
};

struct ImplCallStackEntry final {
  MaroonStateIndex current_idx_;

  std::string f_;
  MaroonVarIndex call_retval_var_idx_;
  std::vector<ImplVar> vars_;

  size_t args_used_ = 0u;
  std::vector<uint64_t> args_;  // TODO(dkorolev): Obviously, no `uint64_t`!

  ImplCallStackEntry() = delete;
  explicit ImplCallStackEntry(MaroonStateIndex idx,
                              std::string f = "",
                              MaroonVarIndex call_retval_var_idx = static_cast<MaroonVarIndex>(-1))
      : current_idx_(idx), f_(std::move(f)), call_retval_var_idx_(call_retval_var_idx) {}

  ImplCallStackEntry(ImplCallStackEntry const&) = default;
  ImplCallStackEntry& operator=(ImplCallStackEntry const&) = default;
};

struct ImplEnv final {
  std::ostream& os_;

  std::vector<ImplCallStackEntry> call_stack_;

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
    oss << expr << '=' << std::forward<T>(v);
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

  void do_debug_dump_vars(std::ostringstream& oss, std::vector<ImplVar> const& vars, char const* const file, int line) {
    oss << '[';
    bool first = true;
    for (auto const& v : vars) {
      if (first) {
        first = false;
      } else {
        oss << ',';
      }
      // TODO(dkorolev): Proper dynamic dispatch to print the values of vars!
      oss << v.name << ':' << static_cast<uint64_t>(v.value);
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

  // TODO(dkorolev): Templated types for vars!
  void DeclareVar(size_t idx, std::string name, uint64_t init) {
    if (idx != call_stack_.back().vars_.size()) {
      std::cerr << "Internal invariant error: corrupted stack." << std::endl;
      std::exit(1);
    }
    ImplVar var;
    var.name = std::move(name);
    // TODO(dkorolev): This will change.
    var.value = static_cast<VarValue>(init);
    call_stack_.back().vars_.push_back(std::move(var));
  }

  void DeclareFunctionArg(size_t idx, std::string name) {
    if (idx != call_stack_.back().vars_.size()) {
      std::cerr << "Internal invariant error: corrupted stack." << std::endl;
      std::exit(1);
    }
    if (call_stack_.back().args_used_ >= call_stack_.back().args_.size()) {
      std::cerr << "Internal invariant error: not enough args, should never happen." << std::endl;
      std::exit(1);
    }
    ImplVar var;
    var.name = std::move(name);
    // TODO(dkorolev): Support other var types.
    var.value = static_cast<VarValue>(call_stack_.back().args_[call_stack_.back().args_used_++]);
    call_stack_.back().vars_.push_back(std::move(var));
  }

  uint64_t& AccessVar(size_t idx, char const* const name) {
    if (idx >= call_stack_.back().vars_.size()) {
      std::cerr << "Internal invariant error: var out of stack." << std::endl;
      std::exit(1);
    }
    if (call_stack_.back().vars_[idx].name != name) {
      std::cerr << "Internal invariant error: corrupted stack, at index " << idx << " expecting var " << name
                << ", have var " << call_stack_.back().vars_[idx].name << std::endl;
      std::exit(1);
    }
    // TODO(dkorolev): This will change.
    return reinterpret_cast<uint64_t&>(call_stack_.back().vars_[idx].value);
  }
};

using step_function_t = void (*)(ImplEnv& env, ImplResultCollector& result);
using vars_function_t = void (*)(ImplEnv& env);

struct MaroonStep final {
  step_function_t code;
  size_t num_vars_available_before_step;
  size_t num_vars_declared_for_step;
  vars_function_t new_vars;
};

#define DEBUG(s) MAROON_env.debug(s, __FILE__, __LINE__)
#define DEBUG_EXPR(s) MAROON_env.debug_expr(#s, s, __FILE__, __LINE__)
#define DEBUG_DUMP_VARS() MAROON_env.debug_dump_vars(__FILE__, __LINE__)
#define DEBUG_DUMP_STACK() MAROON_env.debug_dump_stack(__FILE__, __LINE__)
#define NEXT() MAROON_result.next()
#define DONE() MAROON_result.done()

// NOTE(dkorolev): The ugly yet functional way to tell 1-arg vs. 2-args macros.
#define CALL_DISPATCH(_1, _2, _3, NAME, ...) NAME
#define CALL(...) CALL_DISPATCH(__VA_ARGS__, CALL3, CALL2, NONEXISTENT_CALL1)(__VA_ARGS__)

#define CALL2(f, args) MAROON_result.call_ignore_return(NUMBER_OF_ARGS_##f, FN_##f, #f, pack_args args)
#define CALL3(v, f, args) \
  MAROON_result.call_capture_return(MAROON_VAR_INDEX_##v, NUMBER_OF_ARGS_##f, FN_##f, #f, pack_args args)

#define RETURN(...) MAROON_result.ret(__VA_ARGS__)

template <class T_MAROON, class T_FIBER>
struct MaroonEngine final {
  std::pair<std::string, std::string> run() {
    try {
      std::ostringstream oss;
      ImplEnv env(oss);

      static_assert(T_MAROON::kIsMaroon);
      // TODO(dkorolev): Perhaps add a `static_assert` that this `T_FIBER` is from the right `T_MAROON`.

      // NOTE(dkorolev): This will not compile if there's no `main` in the `global` fiber.
      static_assert(T_FIBER::kIsFiber);

      static_assert(T_FIBER::NUMBER_OF_ARGS_main == 0);

      // TODO(dkorolev): Proper engine =)
      env.call_stack_.push_back(ImplCallStackEntry(T_FIBER::FN_main));
      while (true) {
        if (env.call_stack_.empty()) {
          // TODO(dkorolev): Soon, `RETURN()` should be the right way to exit, not `DONE()`.
          std::cerr << "Internal invariant failed: not in any function somehow." << std::endl;
        }

        if (static_cast<uint32_t>(env.call_stack_.back().current_idx_) >= T_FIBER::kStepsCount) {
          CURRENT_THROW(ImplException("NEXT() out of bounds."));
        }
        MaroonStep const& step = T_FIBER::kSteps[static_cast<uint32_t>(env.call_stack_.back().current_idx_)];

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

        ImplResultCollector result;
        step.code(env, result);

        if (result.status() == TmpNextStatus::Done) {
          break;
        } else if (result.status() == TmpNextStatus::Next) {
          env.call_stack_.back().current_idx_ =
              static_cast<MaroonStateIndex>(static_cast<uint32_t>(env.call_stack_.back().current_idx_) + 1);
        } else if (result.status() == TmpNextStatus::Branch) {
          env.call_stack_.back().current_idx_ = static_cast<MaroonStateIndex>(result.next_idx_);
        } else if (result.status() == TmpNextStatus::Call) {
          env.call_stack_.back().current_idx_ =
              static_cast<MaroonStateIndex>(static_cast<uint32_t>(env.call_stack_.back().current_idx_) + 1);
          env.call_stack_.push_back(
              ImplCallStackEntry(result.call_idx_, std::move(result.call_f_), result.call_retval_var_idx_));
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
