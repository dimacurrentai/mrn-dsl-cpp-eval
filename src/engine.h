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

enum TmpNextStatus { None = 0, Next, Done };
struct ImplResultCollector final {
  TmpNextStatus status_ = TmpNextStatus::None;
  void next() {
    if (status_ != TmpNextStatus::None) {
      CURRENT_THROW(ImplException("TODO(dkorolev): FIXME: Attempted `NEXT()` in the wrong place."));
    }
    status_ = TmpNextStatus::Next;
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

struct ImplEnv final {
  std::ostream& os_;

  explicit ImplEnv(std::ostream& os) : os_(os) {}

  template <typename T>
  void debug(T&& v) {
    std::ostringstream oss;
    oss << std::forward<T>(v);
    std::string const s = oss.str();
    std::cerr << "Impl DEBUG: " << s << std::endl;
    // TODO(dkorolev): Tick index / time.
    os_ << s << std::endl;
  }
};

using step_function_t = void (*)(ImplEnv& env, ImplResultCollector& result);

// TODO(dkorolev): Add "death" tests that require `.next()`, `.done()`, etc.
#define DEBUG(s) env.debug(s)
#define NEXT() result.next()
#define DONE() result.done()

enum class MaronStateIndex : uint32_t;

template <class T_MAROON>
struct MaroonEngine final {
  std::pair<std::string, std::string> run() {
    try {
      std::ostringstream oss;
      ImplEnv env(oss);

      static_assert(T_MAROON::kIsMaroon);
      using T_FIBER = typename T_MAROON::global;

      // NOTE(dkorolev): This will not compile if there's no `main` in the `global` fiber.
      static_assert(T_FIBER::kIsFiber);

      // TODO(dkorolev): Proper engine =)
      auto current_index = static_cast<size_t>(T_FIBER::main);
      while (true) {
        if (current_index >= T_FIBER::kStepsCount) {
          CURRENT_THROW(ImplException("NEXT() out of bounds."));
        }
        step_function_t const& f = T_FIBER::kSteps[current_index];

        ImplResultCollector result;
        f(env, result);

        if (result.status() == TmpNextStatus::Done) {
          break;
        } else {
          ++current_index;
        }
      }

      return {oss.str(), ""};
    } catch (ImplException const& e) {
      return {"", e.OriginalDescription()};
    }
  }
};
