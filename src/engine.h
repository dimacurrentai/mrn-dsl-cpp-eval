// NOTE(dkorolev): This is the somewhat ugly piece of code to "execute" the "post-DSL" boilerplate.

#pragma once

#include <vector>
#include <functional>
#include <iostream>
#include <sstream>

#include "current/bricks/exception.h"

// TODO(dkorolev): Populate this. Enumerators? Strings/names?
struct ImplMaroon {
 protected:
  ~ImplMaroon() = default;
};

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

  void debug(std::string s) {
    std::cerr << "Impl DEBUG: " << s << std::endl;
    // TODO(dkorolev): Tick index / time.
    os_ << s << std::endl;
  }
};

struct ImplStatement final {
  using stmt_t = std::function<void(ImplEnv&, ImplResultCollector&)>;

  stmt_t stmt_;
  ImplStatement(stmt_t stmt) : stmt_(std::move(stmt)) {}
};

// NOTE(dkorolev): In macros expansion, `clang-format` decouples the `&` from the type. Undesirable.
// clang-format off
#define ImplStmt(body) \
  ImplStatement([](ImplEnv& env, ImplResultCollector& result) body)
// clang-format on

// TODO(dkorolev): Add "death" tests that require `.next()`, `.done()`, etc.
#define DEBUG(s) env.debug(s)
#define NEXT() result.next()
#define DONE() result.done()

// TODO(dkorolev): Heap type? Stack types and subtypes? List of functions?
struct ImplFiber {
 protected:
  ~ImplFiber() = default;
};

struct ImplFunction {
  std::vector<ImplStatement> body_;
  ImplFunction(std::vector<ImplStatement> body) : body_(std::move(body)) {}

 protected:
  ~ImplFunction() = default;
};

template <class T_MAROON>
struct MaroonEngine final {
  std::pair<std::string, std::string> run() {
    try {
      std::ostringstream oss;
      ImplEnv env(oss);

      // NOTE(dkorolev): This will not compile if there's no `main` in the `global` fiber.
      typename T_MAROON::global::main main;
      ImplFunction& main_fiber = main;

      // TODO(dkorolev): Proper engine =)
      size_t current_index = 0u;
      while (true) {
        if (current_index >= main_fiber.body_.size()) {
          CURRENT_THROW(ImplException("NEXT() out of bounds."));
        }
        ImplStatement& stmt = main_fiber.body_[current_index];

        ImplResultCollector result;
        stmt.stmt_(env, result);

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
