// NOTE(dkorolev): This is suboptimal, but it ensures the code builds just with `g++ src.cc`, w/o `-std=c++17`.
#define CURRENT_FOR_CPP14

#include <iostream>
#include <fstream>

#include "../current/bricks/dflags/dflags.h"
#include "../current/bricks/file/file.h"
#include "../current/typesystem/serialization/json.h"

#include "ir.h"

DEFINE_string(a, "", "One IR file as JSON.");
DEFINE_string(b, "", "Another IR file as JSON.");
DEFINE_bool(verbose, false, "Actually dump post-line-nullified JSONs.");

void ZeroLineNumbers(MaroonIRScenarios& m);

int main(int argc, char** argv) {
  ParseDFlags(&argc, &argv);

  if (FLAGS_a.empty() || FLAGS_b.empty()) {
    std::cerr << "The `--a` and `--b` parameters are required." << std::endl;
    std::exit(1);
  }

  MaroonIRScenarios a;
  MaroonIRScenarios b;
  using T = decltype(a);

  try {
    a = ParseJSON<T, JSONFormat::Minimalistic>(current::FileSystem::ReadFileAsString(FLAGS_a));
  } catch (current::Exception const&) {
    std::cerr << "Failed to read and parse the IR JSON from `" << FLAGS_a << "`." << std::endl;
    std::exit(1);
  }

  try {
    b = ParseJSON<T, JSONFormat::Minimalistic>(current::FileSystem::ReadFileAsString(FLAGS_b));
  } catch (current::Exception const&) {
    std::cerr << "Failed to read and parse the IR JSON from `" << FLAGS_b << "`." << std::endl;
    std::exit(1);
  }

  // NOTE(dkorolev): The `__LINE__` token prints different lines for multiline parameters.
  ZeroLineNumbers(a);
  ZeroLineNumbers(b);

  // Poor man's comparison.
  std::string const sa = JSON<JSONFormat::Minimalistic>(a);
  std::string const sb = JSON<JSONFormat::Minimalistic>(b);
  if (sa != sb) {
    std::cout << "The IR JSONs are not identical." << std::endl;
    if (FLAGS_verbose) {
      std::cout << std::endl << sa << std::endl << sb << std::endl << std::endl;
    }
    std::exit(1);
  }
}

inline void ZeroLineNumbers(MaroonIRScenarios& m) {
  struct Visitor final {
    void operator()(MaroonIRScenarios& m) {
      for (auto& kv : m.maroon) {
        (*this)(kv.second);
      }
      for (auto& v : m.tests) {
        v.Call(*this);
      }
    }

    void operator()(MaroonIRNamespace& m) {
      m.line = 0;
      for (auto& kv : m.fibers) {
        (*this)(kv.second);
      }
      for (auto& kv : m.types) {
        kv.second.line = 0;
      }
    }

    void operator()(MaroonIRFiber& m) {
      m.line = 0;
      for (auto& kv : m.functions) {
        (*this)(kv.second);
      }
    }

    void operator()(MaroonIRFunction& m) {
      m.line = 0;
      (*this)(m.body);
    }

    void operator()(MaroonIRBlock& m) {
      m.line = 0;
      for (auto& v : m.vars) {
        (*this)(v);
      }
      for (auto& v : m.code) {
        (*this)(v);
      }
    }

    void operator()(MaroonIRStmtOrBlock& m) { m.Call(*this); }

    void operator()(MaroonIRBlockPlaceholder&) {}

    void operator()(MaroonIRStmt& m) { m.line = 0; }

    void operator()(MaroonIRIf& m) {
      m.line = 0;
      (*this)(m.yes);
      (*this)(m.no);
    }

    void operator()(MaroonIRMatchEnumStmt& m) {
      m.line = 0;
      for (auto& a : m.arms) {
        (*this)(a);
      }
    }

    void operator()(MaroonIRMatchEnumStmtArm& a) { a.line = 0; }

    void operator()(MaroonIRVar& v) { v.Call(*this); }

    void operator()(MaroonIRVarRegular& v) { v.line = 0; }
    void operator()(MaroonIRVarFunctionArg& v) { v.line = 0; }
    void operator()(MaroonIRVarEnumCaseCapture&) {}

    void operator()(MaroonTestCaseRunFiber& m) { m.line = 0; }
    void operator()(MaroonTestCaseFiberShouldThrow& m) { m.line = 0; }
  };

  Visitor()(m);
}
