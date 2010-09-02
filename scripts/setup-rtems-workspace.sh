#!/bin/bash
# 
# Run from a simics workspace.  Uses the rtemssparc64 repository.
#

if [[ $# -ne 2 ]]
then
  echo "Error - parameters missing"
  echo "Syntax: $0 gems-top-directory rtems-source"
  echo "Example: $0 ${HOME}/svn/gems ${HOME}/rtems/rtems"
  exit 1
fi

# Check for simics workspace
if [[ ! -f simics || ! -d targets || ! -f compiler.mk ]]
then
  echo "Current directory is not a simics workspace!"
  exit 1
fi

# Check that $1 is gems top-level directory
if [[ ! -d $1/gems-2.1.1 || ! -d $1/support ]]
then
  echo "$1 is not the top-level of the gems repository!"
  exit 1
fi

# Check that $2 is a valid rtems source directory
if [[ ! -d $2/cpukit || ! -d $2/c ]]
then
  echo "$2 is not a valid rtems source-code directory"
  exit 1
fi

WORKSPACE=`pwd`
TOP=$1
RTEMS=$2
SUPPORT=${TOP}/support

# Check for build-sparc64 directory. If it exists, don't re-create it.
if [[ ! -d build-sparc64 ]]
then
  tar -zxf ${SUPPORT}/build-sparc64.tgz
fi

cd build-sparc64
if [[ ! -h rtems ]]
then
  ln -s ${RTEMS} rtems
fi

touch boot/image.iso

cd ${WORKSPACE}
cp  ${SUPPORT}/runRTEMStest-gems.sh .
cp  ${SUPPORT}/runRTEMStest-opal.sh .
cp  ${SUPPORT}/runRTEMStest-usiii.sh .
cp  ${SUPPORT}/gems-quicktest.sh .
cp  ${SUPPORT}/opal-quicktest.sh .
cp  ${SUPPORT}/usiii-quicktest.sh .

cd ${WORKSPACE}/targets/serengeti
cp  ${SUPPORT}/sun4u.rtems4.11-template-gems.simics .
cp  ${SUPPORT}/sun4u.rtems4.11-template-opal.simics .
cp  ${SUPPORT}/sun4u.rtems4.11-template-plain.simics .

cd ${WORKSPACE}
mkdir output
ln build-sparc64/boot/image.iso .
if [[ ! -h sources ]]
then
  ln -s ${RTEMS} sources
fi

if [[ ! -h symbols ]]
then
  ln -s build-sparc64/boot/debug/rtems symbols
fi

echo "RTEMS workspace setup complete"

