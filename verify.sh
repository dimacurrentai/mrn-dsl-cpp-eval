#!/bin/bash

# NOTE(dkorolev): This file is run as the Github check, and it's best to install it as a git hook via:

set -e

CMD="""
ln -sf "../../verify.sh" ".git/hooks/pre-commit"
"""

echo 'Running `./verify.sh` ...'

for i in *.mrn ; do
  ./mrn2ir.sh "$i" --verify
done

for i in autogen/*.mrn.json ; do
  MRN="${i#autogen/}"
  MRN="${MRN/%.json}"
  if ! [ -f "$MRN" ] ; then
    echo "Seeing '$i' but no '$MRN', you may have moved or deleted some source without cleaning up 'autogen/'."
    exit 1
  fi
done

echo 'Running `./verify.sh` : Success.'
