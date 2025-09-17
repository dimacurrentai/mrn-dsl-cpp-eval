#!/bin/bash

set -e

if [ "$1" == "" ] ; then
  echo 'Need an argument, the `.mrn` file in this directory that has had `mrn2ir.sh` run on already.'
  exit 1
fi

IN="${1%.mrn}"

if ! [ -f "autogen/$IN.mrn.json" ] ; then
  echo "The autogen/$IN.mrn.json file should exist."
  exit 1
fi

./autogen/ir2cpp.bin --in "autogen/$IN.mrn.json" --name "$IN" --out "autogen/$IN.mrn.test.h"
