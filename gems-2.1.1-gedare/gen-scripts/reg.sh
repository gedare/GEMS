#!/bin/sh

REGRESS=false
CHECKOUT=false
export REGRESS
export CHECKOUT
cd `dirname $0`
cd ../../../
./multifacet/condor/gen-scripts/regress.py

