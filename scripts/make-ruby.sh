#!/bin/bash

# This should be run inside of a gems-2.1.1-XXX directory.
if [[ ! -d ruby ]]
then
  echo "Error - ruby not found."
  echo "Not in a valid GEMS directory."
  echo "Make sure to run this script from inside of a gems-x.y.z-XXX directory"
  exit 1
fi

cd ruby
make PROTOCOL=MOSI_SMP_bcast DESTINATION=MOSI_SMP_bcast

