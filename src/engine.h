// NOTE(dkorolev): This is the somewhat ugly piece of code to "execute" the "post-DSL" boilerplate.

#pragma once

#include <vector>
#include <functional>
#include <iostream>
#include <sstream>

#include "../current/bricks/exception.h"

// TODO(dkorolev): If we agree it's uint32_t, need to make sure the future compiler checks the size of the program.
enum class MaroonStateIndex : uint32_t;

struct ImplException : current::Exception {
  using current::Exception::Exception;
};

enum TmpNextStatus { None = 0, Next, Branch, Done, Call, Return };
struct ImplResultCollector final {
  MaroonStateIndex next_idx_;
  TmpNextStatus status_ = TmpNextStatus::None;
  MaroonStateIndex call_idx_;
  std::string call_f_;
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
  void call(MaroonStateIndex idx, std::string f) {
    status_ = TmpNextStatus::Call;
    call_idx_ = idx;
    call_f_ = std::move(f);
  }
  void ret() { status_ = TmpNextStatus::Return; }
  TmpNextStatus status() const {
    if (status_ == TmpNextStatus::None) {
      CURRENT_THROW(ImplException("`AWAIT` or `RETURN` condition missing in an `STMT`."));
    }
    return status_;
  }
};

// TODO(dkorolev): Support different variable types =) and overall the Maroon stack here.
enum class VarValue : uint64_t;

struct ImplVar final {
  std::string name;
  VarValue value;
};

struct ImplCallStackEntry final {
  std::string f_;
  MaroonStateIndex current_idx_;
  std::vector<ImplVar> vars_;
  ImplCallStackEntry() = delete;
  explicit ImplCallStackEntry(MaroonStateIndex idx, std::string f = "") : current_idx_(idx), f_(std::move(f)) {}
  ImplCallStackEntry(ImplCallStackEntry const&) = default;
  ImplCallStackEntry& operator=(ImplCallStackEntry const&) = default;
  // TODO(dkorolev): The pointer to the variable on stack of the caller where the result should be stored!
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

#define DEBUG(s) env.debug(s, __FILE__, __LINE__)
#define DEBUG_EXPR(s) env.debug_expr(#s, s, __FILE__, __LINE__)
#define DEBUG_DUMP_VARS() env.debug_dump_vars(__FILE__, __LINE__)
#define DEBUG_DUMP_STACK() env.debug_dump_stack(__FILE__, __LINE__)
#define NEXT() result.next()
#define DONE() result.done()
#define CALL(f) result.call(FN_##f, #f)
#define RETURN(...) result.ret(__VA_ARGS__)

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
          env.call_stack_.push_back(ImplCallStackEntry(result.call_idx_, std::move(result.call_f_)));
        } else if (result.status() == TmpNextStatus::Return) {
          env.call_stack_.pop_back();
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
