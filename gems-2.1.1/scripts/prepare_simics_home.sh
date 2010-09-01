#!/bin/bash

# this script prepare a simics home directory

function relink_if_link {
  if [ -h $2 ]; then
    rm $2
  fi
  if [ ! -e $2 ]; then
    ln -s $1 $2
  fi
}

if [ ! -z $1 ]; then
  if [ ! -d "$1" ]; then
    mkdir "$1"
  fi
fi

if [ -d "$1" ]; then
  if [ -n "$2" ]; then
    cd "$1"
    mkdir -p "$2"/modules 
    mkdir -p "$2"/modules/python 
    relink_if_link ../../../x86-linux/lib "$2"/lib
    relink_if_link "$2"/lib lib
    relink_if_link "$2"/modules modules
    # for SPARC
    relink_if_link ../sarek/simics simics
    # for x86
    #relink_if_link ../x86-440bx/simics simics
  else
    echo "usage: prepare_simics_home.sh <home dir> <host type>"
  fi
else
  echo "usage: prepare_simics_home.sh <home dir> <host type>"
fi
