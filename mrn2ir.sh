#!/bin/bash

set -e

if [ "$1" == "" ] ; then
  echo 'Need an argument, the `.mrn` file in this directory.'
  exit 1
fi

IN="${1%.mrn}"

if ! [ -f "$IN.mrn" ] ; then
  echo "The $IN.mrn file should exist."
  exit 1
fi

mkdir -p autogen

# Start preparing the IR-generating code.
cp src/boilerplate/dsl.prefix.h autogen/"$IN.mrn.cc"

# Run the preprocessor on this IR-generating code to turn it into what will build to ultimately produce the JSON IR.
echo '#include "../src/boilerplate/dsl.spec.h"' >"autogen/$IN.mrn.h"
cat "$IN.mrn" >>"autogen/$IN.mrn.h"
g++ -E "autogen/$IN.mrn.h" 2>/dev/null | grep -v '^#' | grep -v '^$' >>autogen/"$IN.mrn.cc"
echo -e "  ;\n  std::cout << JSON<JSONFormat::Minimalistic>(ctx.out) << std::endl;\n}" >>autogen/"$IN.mrn.cc"

# TODO(dkorolev): I'm using `clang-format` here, need to make sure it exists!
clang-format -i autogen/"$IN.mrn.cc"

# Build and run the source file that was just put together to generate the JSON IR.
g++ -std=c++17 autogen/"$IN.mrn.cc" -o autogen/"$IN.mrn.bin" && autogen/"$IN.mrn.bin" | jq . >autogen/"$IN.mrn.json"

# Remove the now-unneeded "original header file".
rm -f "autogen/$IN.mrn.h"

# TODO(dkorolev: Introduce `jq` here same as `clang-format`, in a safe way.
