#!/bin/bash

set -e

make all

cat <<EOF >autogen/test.cc
#define CURRENT_FOR_CPP14

#include "../src/current/3rdparty/gtest/gtest-main.h"

#include "../src/ir.h"
#include "../src/current/typesystem/serialization/json.h"

EOF

(cd autogen; for i in *.mrn.test.h ; do echo "#include \"$i\"" >>test.cc;  done)

g++ -std=c++17 autogen/test.cc -o autogen/test.bin

./autogen/test.bin
