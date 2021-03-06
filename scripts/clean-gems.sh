#!/bin/bash

# This should be run inside of a gems-2.1.1-XXX directory.
if [[ ! -d opal ]]
then
  echo "Error - not in a valid GEMS directory."
  echo "Make sure to run this script from inside of a gems-x.y.z-XXX directory"
  exit 1
fi

PWD=`pwd`
export GEMS=${PWD}

cd $GEMS/opal
make clean

cd $GEMS/ruby
make clean

cd $GEMS/slicc
make clean

cd $GEMS

# undo the pre-build patch
patch -R -p1 < ../gems-patches/gems-2.1.1-pre-build.diff

#delete simics softlink
rm simics

# overwrite files modified by make-gems.sh
cp ../gems-2.1.1/scripts/makesymlinks.sh scripts/makesymlinks.sh
cp ../gems-2.1.1/common/Makefile.common common/Makefile.common

