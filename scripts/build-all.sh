#!/bin/bash
#
# If this script fails to build everything for you:
#   Check doc/GEMS-Ubuntu.pdf to ensure pre-requisites.
#   Make sure to apply any necessary patches.
#   Check Simics License server
#   Check RTEMS source code

# Check for parameters.
if [[ $# -ne 2 ]]
then
  echo "Error - parameters missing"
  echo "Syntax : $0 simics_install rtems_source"
  echo "example : $0 ${HOME}/work/simics/simics-3.0.31 ${HOME}/work/rtems/rtems"
  exit 1
fi

# Make sure $1 is a simics installation
if [[ ! -d $1/targets && ! -d $1/scripts ]]
then
  echo "Error - $1 is not a valid simics installation"
  exit 1
fi

# Check that $2 is a valid rtems source directory
if [[ ! -d $2/cpukit || ! -d $2/c ]]
then
  echo "$2 is not a valid rtems source-code directory"
  exit 1
fi

# This should be run inside of a gems-2.1.1-XXX directory.
if [[ ! -d opal ]]
then
  echo "Error - not in a valid GEMS directory."
  echo "Make sure to run this script from inside of a gems-x.y.z-XXX directory"
  exit 1
fi

PWD=`pwd`
export GEMS=${PWD}
RTEMS=$2

cd ..
export GEMS_TOP=`pwd`

cd ${GEMS}

# setup and build gems
${GEMS_TOP}/scripts/setup-gems.sh $1

# setup workspace to build rtems
cd simics
${GEMS_TOP}/scripts/setup-rtems-workspace.sh ${GEMS_TOP} $2

# configure and build rtems
cd build-sparc64
./build-b-usiiicvs.sh

# create the image.iso
cd boot
./build-usiii.sh

# run hello world
cd ../../
./usiii-quicktest.sh samples hello
cat output/hello.txt

