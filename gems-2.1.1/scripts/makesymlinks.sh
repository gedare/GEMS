#!/bin/sh

# for SPARC
echo "Making symlinks for simics/home/sarek spoofing..."
# for x86
#echo "Making symlinks for simics/home/x86-440bx spoofing..."

# ALL targets
ln -s targets home
cd home

# for SPARC
ln -s serengeti sarek
cd sarek
# for x86
#cd x86-440bx

ln -s ../../simics simics
cd ../..

# Edit this link command to link to your import directory
# in your Simics 3 install
echo "Making symlink for import directory..."
#ln -s /simics-3.0.11/import import
ln -s /p/multifacet/projects/simics/simics-3.0.11/import import

echo "Making symlinks for modules..."
cd modules

echo "Making symlinks for Opal..."
mkdir opal
cd opal
ln -s ../../../opal/module/commands.py .
ln -s ../../../opal/system/hfa_init.h .
ln -s ../../../opal/module/Makefile .
ln -s ../../../opal/module/opal.c .
ln -s ../../../opal/module/opal.h .
cd ..

echo "Making symlinks for Ruby..."
mkdir ruby
cd ruby
ln -s ../../../ruby/simics/commands.h .
ln -s ../../../ruby/module/commands.py .
ln -s ../../../ruby/module/Makefile .
ln -s ../../../ruby/interfaces/mf_api.h .
ln -s ../../../ruby/module/ruby.c .
cd ..

echo "Making symlinks for Tourmaline..."
mkdir tourmaline
cd tourmaline
ln -s ../../../tourmaline/simics/commands.h .
ln -s ../../../tourmaline/module/commands.py .
ln -s ../../../tourmaline/module/Makefile .
ln -s ../../../tourmaline/module/tourmaline.c .
cd ..

cd ..

echo "Symlinks made."

#eof
