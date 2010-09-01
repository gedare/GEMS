#!/bin/bash

cd ..
GEMS_ROOT=${PWD}
cd doxygen

sed s:RUBY_PATH_PREFIX:${GEMS_ROOT}/ruby: < gems.config > gems.config2
doxygen gems.config2
