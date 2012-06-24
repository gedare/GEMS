
/*
    Copyright (C) 1999-2008 by Mark D. Hill and David A. Wood for the
    Wisconsin Multifacet Project.  Contact: gems@cs.wisc.edu
    http://www.cs.wisc.edu/gems/

    --------------------------------------------------------------------

    This file is part of the Opal Timing-First Microarchitectural
    Simulator, a component of the Multifacet GEMS (General 
    Execution-driven Multiprocessor Simulator) software toolset 
    originally developed at the University of Wisconsin-Madison.

    Opal was originally developed by Carl Mauer based upon code by
    Craig Zilles.

    Substantial further development of Multifacet GEMS at the
    University of Wisconsin was performed by Alaa Alameldeen, Brad
    Beckmann, Jayaram Bobba, Ross Dickson, Dan Gibson, Pacia Harper,
    Derek Hower, Milo Martin, Michael Marty, Carl Mauer, Michelle Moravan,
    Kevin Moore, Andrew Phelps, Manoj Plakal, Daniel Sorin, Haris Volos, 
    Min Xu, and Luke Yen.
    --------------------------------------------------------------------

    If your use of this software contributes to a published paper, we
    request that you (1) cite our summary paper that appears on our
    website (http://www.cs.wisc.edu/gems/) and (2) e-mail a citation
    for your published paper to gems@cs.wisc.edu.

    If you redistribute derivatives of this software, we request that
    you notify us and either (1) ask people to register with us at our
    website (http://www.cs.wisc.edu/gems/) or (2) collect registration
    information and periodically send it to us.

    --------------------------------------------------------------------

    Multifacet GEMS is free software; you can redistribute it and/or
    modify it under the terms of version 2 of the GNU General Public
    License as published by the Free Software Foundation.

    Multifacet GEMS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with the Multifacet GEMS; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307, USA

    The GNU General Public License is contained in the file LICENSE.

### END HEADER ###
*/
# Makefile 
#

CC := g++

# PURIFY := purify -leaks-at-exit=yes -chain-length="15" 
# PURIFY := purecov
# PURIFY := quantify

PCOV = /s/std/bin/purecov -log-file=%v.log

OPT_FLAGS := -g 
#OPT_FLAGS := -g -O2 -finline-functions

DEBUG_FLAGS := -DDEBUG=true

#INCLUDES := -Icommon -I. -I/usr/openwin/include 

# _REENTRANT is specified as necessary for strtok_r
CFLAGS := -Wall -D_REENTRANT $(OPT_FLAGS) $(BUILD_FLAGS) $(DEBUG_FLAGS) $(INCLUDES)

LIBS :=  -lthread -lsocket -lnsl -ldl

YACC := bison
YFLAGS := -d
LEX := flex
LFLAGS := -i
SHELL := /bin/sh

# all the sources 
SRC_CPP := \
	FrontEnd.C \
	global.C \
	Thread.C \
	ThreadContext.C

#all the object files
OBJ := $(SRC_CPP:.C=.o)
ALLOBJ := $(OBJ)

#########
#begin targets
#########

all:  memtest
pure: memtest.pure

memtest: $(ALLOBJ) memtest.o
	$(CC) -o $@ $(CFLAGS) $(ALLOBJ) memtest.o $(LIBS)

depend:
	makedepend *.C *.h

# -- Generate tags for Emacs
tags: *.[Ch]
	etags *.[Cch]

.y.c:
	$(YACC) $(YFLAGS) $<
	mv $*.tab.c $*.c
	mv $*.tab.h $*.h

.l.c:
	$(LEX) $(LFLAGS) $<
	mv lex.yy.c $*.c

%.o: %.C
	$(CC) $(CFLAGS) -o $@ -c $<

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<
%.o: %.cc
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	/bin/rm -f $(OBJ) memtest memtest.pure
# DO NOT DELETE

