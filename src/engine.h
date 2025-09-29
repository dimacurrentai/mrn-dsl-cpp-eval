// NOTE(dkorolev): This is the somewhat ugly piece of code to "execute" the "post-DSL" boilerplate.

#pragma once

#include <vector>
#include <functional>
#include <iostream>
#include <sstream>

#include "../current/bricks/exception.h"

struct ImplException : current::Exception {
  using current::Exception::Exception;
};

enum TmpNextStatus { None = 0, Next, Branch, Done };
struct ImplResultCollector final {
  TmpNextStatus status_ = TmpNextStatus::None;
  uint32_t idx_;
  void next() {
    if (status_ != TmpNextStatus::None) {
      CURRENT_THROW(ImplException("TODO(dkorolev): FIXME: Attempted `NEXT()` in the wrong place."));
    }
    status_ = TmpNextStatus::Next;
  }
  void branch(uint32_t idx) {
    if (status_ != TmpNextStatus::None) {
      CURRENT_THROW(ImplException("TODO(dkorolev): FIXME: Attempted to `IF()` in the wrong place."));
    }
    status_ = TmpNextStatus::Branch;
    idx_ = idx;
  }
  void done() {
    if (status_ != TmpNextStatus::None) {
      CURRENT_THROW(ImplException("TODO(dkorolev): FIXME: Attempted `DONE()` in the wrong place."));
    }
    status_ = TmpNextStatus::Done;
  }
  TmpNextStatus status() const {
    if (status_ == TmpNextStatus::None) {
      CURRENT_THROW(ImplException("`AWAIT` or `RETURN` condition missing in an `STMT`."));
    }
    return status_;
  }
};

// TODO(dkorolev): Support different variable types =) and overall the Maroon stack here.
struct ImplVar final {
  std::string name;
  uint64_t value;
};

struct ImplEnv final {
  std::ostream& os_;
  std::vector<ImplVar> vars_;

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
    oss << '[';
    bool first = true;
    for (auto const& v : vars_) {
      if (first) {
        first = false;
      } else {
        oss << ',';
      }
      oss << v.name << ':' << v.value;
    }
    oss << ']';
    std::string const s = oss.str();
    std::cerr << "Impl VARS: " << s << " @ " << file << ':' << line << std::endl;
    // TODO(dkorolev): Tick index / time.
    os_ << s << std::endl;
  }

  // TODO(dkorolev): Templated types for vars!
  void DeclareVar(size_t idx, std::string name, uint64_t init) {
    if (idx != vars_.size()) {
      std::cerr << "Internal invariant error: corrupted stack." << std::endl;
      std::exit(1);
    }
    ImplVar var;
    var.name = std::move(name);
    var.value = init;
    vars_.push_back(std::move(var));
  }

  uint64_t& AccessVar(size_t idx, char const* const name) {
    if (idx >= vars_.size()) {
      std::cerr << "Internal invariant error: var out of stack." << std::endl;
      std::exit(1);
    }
    if (vars_[idx].name != name) {
      std::cerr << "Internal invariant error: corrupted stack, at index " << idx << " expecting var " << name
                << ", have var " << vars_[idx].name << std::endl;
      std::exit(1);
    }
    return vars_[idx].value;
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
#define NEXT() result.next()
#define DONE() result.done()

enum class MaronStateIndex : uint32_t;

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
      auto current_index = static_cast<size_t>(T_FIBER::main);
      while (true) {
        if (current_index >= T_FIBER::kStepsCount) {
          CURRENT_THROW(ImplException("NEXT() out of bounds."));
        }
        MaroonStep const& step = T_FIBER::kSteps[current_index];

        if (env.vars_.size() < step.num_vars_available_before_step) {
          std::cerr << "Internal invariant failed: pre-step vars count mismatch." << std::endl;
          std::exit(1);
        }

        if (env.vars_.size() > step.num_vars_available_before_step) {
          // Destruct what is no longer needed.
          env.vars_.resize(step.num_vars_available_before_step);
        }

        step.new_vars(env);

        if (env.vars_.size() != step.num_vars_available_before_step + step.num_vars_declared_for_step) {
          std::cerr << "Internal invariant failed: intra-step vars count mismatch." << std::endl;
          std::exit(1);
        }

        ImplResultCollector result;
        step.code(env, result);

        if (result.status() == TmpNextStatus::Done) {
          break;
        } else if (result.status() == TmpNextStatus::Next) {
          ++current_index;
        } else if (result.status() == TmpNextStatus::Branch) {
          current_index = result.idx_;
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
