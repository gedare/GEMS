#!/bin/bash
#
# If this script fails to build everything for you:
#   Check doc/GEMS-Ubuntu.pdf to ensure pre-requisites.
#   Make sure to apply any necessary patches. (NOTE: the pre-build 
#   patch is applied by this script, and reversed by the clean-gems.sh
#   script.)
#

# Check for parameters.
if [[ $# -ne 1 ]]
then
  echo "Error - parameters missing"
  echo "Syntax : $0 simics_install"
  echo "example : $0 ${HOME}/work/simics/simics-3.0.31"
  exit 1
fi

# Make sure $1 is a simics installation
if [[ ! -d $1/targets && ! -d $1/scripts ]]
then
  echo "Error - $1 is not a valid simics installation"
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
export SIMICS_INSTALL=$1
export GEMS=${PWD}

## Check for the presence of the simics symlink. Its presence indicates 
## that clean-gems.sh was not run, so this is a 'quick' re-build.
if [[ ! -h simics ]]
then
  # instantiate Simics workspace
  if [[ ! -d ${GEMS}/simics_3_workspace ]]
  then
    mkdir ${GEMS}/simics_3_workspace
  fi

  cd ${SIMICS_INSTALL}/bin
  ./workspace-setup $GEMS/simics_3_workspace

  # modify makesymlinks.sh, then run it from inside of simics_3_workspace
  sed -i \
  -e 's/\/p\/multifacet\/projects\/simics\/simics-3.0.11/${SIMICS_INSTALL}/' \
  ${GEMS}/scripts/makesymlinks.sh

  cd ${GEMS}/simics_3_workspace
  ${GEMS}/scripts/makesymlinks.sh

  # create simics soft-link
  cd ${GEMS}
  ln -s simics_3_workspace simics

  # update common/Makefile.common
  sed -i \
  -e 's/\/s\/gcc-3.4.4\/bin\/g++/\/usr\/bin\/g++/' \
  -e 's/\/s\/gcc-3.4.1\/bin\/g++/\/usr\/bin\/g++/' \
  -e 's/\/dev\/null30/$(GEMS_ROOT)\/simics/' \
  -e 's/$(GEMS_ROOT)\/simics\/src\/include/$(SIMICS_INSTALL)\/src\/include/' \
  ${GEMS}/common/Makefile.common

  # apply pre-build patch.
  cd $GEMS
  patch -p1 < ../patches/gems-2.1.1-pre-build.diff
fi

#cd $GEMS/ruby
#make PROTOCOL=MOSI_SMP_bcast DESTINATION=MOSI_SMP_bcast

cd $GEMS/opal
make module DESTINATION=MOSI_SMP_bcast
