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

// TODO(dkorolev): Catch them and test for those too!
struct ImplException : current::Exception {
  using current::Exception::Exception;
};

struct ImplResultCollector final {
  // TODO(dkorolev): Throw `ImplException`-s on conflicting (or missing!) commands.
  bool is_next = false;
  bool is_done = false;
  void next() { is_next = true; }
  void done() { is_done = true; }
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

// TODO(dkorolev): Update `.clang-format` to have `ImplEnv& env` format as this. In `C5T/Current`!
#define ImplStmt(body) \
  ImplStatement([](ImplEnv& env, ImplResultCollector& result) body)

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
  std::string const report;
  MaroonEngine() : report(run()) {}

 private:
  static std::string run() {
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
          CURRENT_THROW(ImplException("Should not `next();` from the last `STMT()`."));
        }
        ImplStatement& stmt = main_fiber.body_[current_index];

        ImplResultCollector result;
        stmt.stmt_(env, result);

        if (result.is_done) {
          break;
        } else {
          ++current_index;
        }
      }

      return oss.str();
    } catch (ImplException const& e) {
      return "EXCEPTION: " + e.DetailedDescription();
    }
  }
};
