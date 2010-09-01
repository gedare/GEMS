#!/bin/bash

if [[ $# -ne 2 ]]
then
  echo "Error - parameters missing"
  echo "Syntax : $0 test_group test_name"
  echo "example : $0 samples hello"
  exit 1
fi

./runRTEMStest-opal.sh output image.iso $1 $2 sources symbols

