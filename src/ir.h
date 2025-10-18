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

// TODO(dkorolev): Add a `make` target to generate the `.md` describing this schema.

CURRENT_STRUCT(MaroonIRVar) {
  CURRENT_FIELD(line, uint32_t);
  CURRENT_FIELD(name, std::string);
  CURRENT_FIELD(type, std::string);            // NOTE(dkorolev): Would love to `enum` this somehow.
  CURRENT_FIELD(init, Optional<std::string>);  // NOTE(dkorolev): Not sure I like this as `string`, but works for now.
};

CURRENT_FORWARD_DECLARE_STRUCT(MaroonIRStmt);
CURRENT_FORWARD_DECLARE_STRUCT(MaroonIRIf);
CURRENT_FORWARD_DECLARE_STRUCT(MaroonIRBlock);
CURRENT_STRUCT(MaroonIRBlockPlaceholder) {  // NOTE(dkorolev): To avoid pointers.
  CURRENT_FIELD(line, uint32_t);
  CURRENT_FIELD(_idx, uint32_t);
};
CURRENT_VARIANT(MaroonIRStmtOrBlock, MaroonIRStmt, MaroonIRIf, MaroonIRBlock, MaroonIRBlockPlaceholder);

// A piece of "O(1)" code to execute.
// TODO(dkorolev): Handle the `AWAIT`-condition separately here, on the type system level.
// TODO(dkorolev): As in, add fields for `await`, a variant of `await / next / done`.
CURRENT_STRUCT(MaroonIRStmt) {
  CURRENT_FIELD(line, uint32_t);
  CURRENT_FIELD(stmt, std::string);
};

CURRENT_STRUCT(MaroonIRIf) {
  CURRENT_FIELD(line, uint32_t);
  CURRENT_FIELD(cond, std::string);
  CURRENT_FIELD(yes, MaroonIRStmtOrBlock);
  CURRENT_FIELD(no, MaroonIRStmtOrBlock);
};

// A set of variables plus the sequence of statements, possibly nested.
// TODO(dkorolev): We now have hoisting, like in the 1st version of JavaScript, lolwut! Fix this.
CURRENT_STRUCT(MaroonIRBlock) {
  CURRENT_FIELD(line, uint32_t);
  CURRENT_FIELD(vars, std::vector<MaroonIRVar>);
  CURRENT_FIELD(code, std::vector<MaroonIRStmtOrBlock>);
};

CURRENT_STRUCT(MaroonIRFunction) {
  CURRENT_FIELD(line, uint32_t);
  // TODO(dkorolev): Types of arguments!
  CURRENT_FIELD(number_of_args, uint32_t, 0);
  CURRENT_FIELD(body, MaroonIRBlock);
};

CURRENT_STRUCT(MaroonIRFiber) {
  CURRENT_FIELD(line, uint32_t);
  // TODO(dkorolev): Heap type.
  CURRENT_FIELD(functions, (std::map<std::string, MaroonIRFunction>));
};

CURRENT_STRUCT(MaroonIRNamespace) {
  CURRENT_FIELD(line, uint32_t);
  // TODO(dkorolev): Support types, heaps, etc.
  // CURRENT_FIELD(types, ...);
  // NOTE(dkorolev): The `global` fiber should absolutely exist, others optional.
  CURRENT_FIELD(fibers, (std::map<std::string, MaroonIRFiber>));
};

CURRENT_STRUCT(MaroonTestCaseRunFiber) {
  CURRENT_FIELD(line, uint32_t);
  CURRENT_FIELD(maroon, std::string);
  CURRENT_FIELD(fiber, std::string);
  CURRENT_FIELD(golden_output, std::vector<std::string>);
};

CURRENT_STRUCT(MaroonTestCaseFiberShouldThrow) {
  CURRENT_FIELD(line, uint32_t);
  CURRENT_FIELD(maroon, std::string);
  CURRENT_FIELD(fiber, std::string);
  CURRENT_FIELD(error, std::string);
};

CURRENT_VARIANT(MaroonTestCase, MaroonTestCaseRunFiber, MaroonTestCaseFiberShouldThrow);

CURRENT_STRUCT(MaroonIRScenarios) {
  CURRENT_FIELD(src, std::string);
  CURRENT_FIELD_DESCRIPTION(src, "The source `.mrn` file.");
  CURRENT_FIELD(maroon, (std::map<std::string, MaroonIRNamespace>));
  CURRENT_FIELD(tests, std::vector<MaroonTestCase>);
};

using MaroonIRTopLevel = MaroonIRScenarios;
