#!/bin/bash

if [[ $# -ne 2 ]]
then
  echo "Error - parameters missing"
  echo "Syntax: $0 new_gems_dir patchname"
  echo "Example: $0 gems-2.1.1-generic generic.diff"
  exit 1
fi

if [[ ! -f dontdiff || ! -d gems-2.1.1 ]]
then
  echo "Error: Are you in the right directory?"
  echo "This should be run in the top-level directory of this repo."
  exit 1
fi

diff -X dontdiff -uprN gems-2.1.1 $1 > $2

