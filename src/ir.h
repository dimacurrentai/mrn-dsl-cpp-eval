// This is the schema for the IR.
//
// Life of a test from this directory: (TODO(dkorolev): This directory, or perhaps `cases/`?)
// 1. First from the DSL into the IR-generating code (via the C preprocessor).
// 2. Then from the IR-generating code to the JSON with this DSL file in the IR format (by running the generating code).
// 3. Then from this JSON to the new piece of code, C++ for now, which will be executed (by running `autogen.cc`).
// 4. And finally the resulting generated C++ code is run, as the unit test — because it is a unit test!
//
// TODO(dkorolev): Explain directories and what builds what.
// TODO(dkorolev): Have this header copied to every source/header file here.
// TODO(dkorolev): Turn the above into README.md for this directory and reference it.

#pragma once

#include "../current/typesystem/struct.h"
#include "../current/typesystem/variant.h"

// TODO(dkorolev): MOVE TO GENERATOR CODE!
// #include "current/typesystem/serialization/json.h"

// TODO(dkorolev): Add a `make` target to generate the `.md` describing this schema.
// TODO(dkorolev): Add git hooks and a GitHub action to validate that everything is autogen'd properly.

CURRENT_FORWARD_DECLARE_STRUCT(MaroonIRNop);
CURRENT_FORWARD_DECLARE_STRUCT(MaroonIRCode);
CURRENT_FORWARD_DECLARE_STRUCT(MaroonIRBlock);
CURRENT_FORWARD_DECLARE_STRUCT(MaroonIRSeq);
CURRENT_VARIANT(MaroonIRStatement, MaroonIRNop, MaroonIRCode, MaroonIRBlock, MaroonIRSeq);

// An "empty body of code", mostly to make the generating code simpler.
CURRENT_STRUCT(MaroonIRNop){};

// A piece of "O(1)" code to execute.
// TODO(dkorolev): Perhaps handle the `AWAIT`-condition separately here.
CURRENT_STRUCT(MaroonIRCode) { CURRENT_FIELD(code, std::string); };

// A code statement with variables declared within it.
CURRENT_STRUCT(MaroonIRBlock) {
  // TODO(dkorolev): Add `vars`.
  CURRENT_FIELD(stmt, MaroonIRStatement);
};

// A sequence of statements.
CURRENT_STRUCT(MaroonIRSeq) { CURRENT_FIELD(seq, std::vector<MaroonIRStatement>); };

// TODO(dkorolev): Wrap this into a `CURRENT_STRUCT`, it has arguments!
using MaroonIRFunction = MaroonIRStatement;

CURRENT_STRUCT(MaroonIRFiber) {
  // TODO(dkorolev): Heap.
  CURRENT_FIELD(functions, (std::map<std::string, MaroonIRFunction>));
};

CURRENT_STRUCT(MaroonIRNamespace) {
  // TODO(dkorolev): Support types, heaps, etc.
  // CURRENT_FIELD(types, ...);
  // NOTE(dkorolev): The `global` fiber should absolutely exist, others optional.
  CURRENT_FIELD(fibers, (std::map<std::string, MaroonIRFiber>));
};

CURRENT_STRUCT(MaroonDebugStatement) {
  CURRENT_FIELD(ts, uint64_t);
  CURRENT_FIELD(msg, std::string);
};

CURRENT_STRUCT(MaroonTestCaseSimple) {
  // TODO(dkorolev): Add logical timestamps here.
  CURRENT_FIELD(maroon, std::string);
  CURRENT_FIELD(debug_statements, std::vector<MaroonDebugStatement>);
};

CURRENT_STRUCT(MaroonTestCaseShouldThrow) {
  CURRENT_FIELD(maroon, std::string);
  CURRENT_FIELD(error, std::string);
};

// TODO(dkorolev): Support more complex testing.
CURRENT_VARIANT(MaroonTestCase, MaroonTestCaseSimple, MaroonTestCaseShouldThrow);

CURRENT_STRUCT(MaroonIRScenarios) {
  CURRENT_FIELD(maroon, (std::map<std::string, MaroonIRNamespace>));
  CURRENT_FIELD(tests, std::vector<MaroonTestCase>);
};
