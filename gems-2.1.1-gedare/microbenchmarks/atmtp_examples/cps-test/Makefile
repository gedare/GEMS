# Copyright (C) 2007 Sun Microsystems, Inc.  All rights reserved.
# U.S. Government Rights - Commercial software.  Government users are
# subject to the Sun Microsystems, Inc. standard license agreement and
# applicable provisions of the FAR and its supplements.  Use is
# subject to license terms.  This distribution may include materials
# developed by third parties.Sun, Sun Microsystems and the Sun logo
# are trademarks or registered trademarks of Sun Microsystems, Inc. in
# the U.S. and other countries.  All SPARC trademarks are used under
# license and are trademarks or registered trademarks of SPARC
# International, Inc. in the U.S.  and other countries.
#
# ----------------------------------------------------------------------
#
# This file is part of the Adaptive Transactional Memory Test Platform
# (ATMTP) developed and maintained by Kevin Moore and Dan Nussbaum of
# the Scalable Synchronization Research Group at Sun Microsystems
# Laboratories (http://research.sun.com/scalable/).  For information
# about ATMTP, see the GEMS website: http://www.cs.wisc.edu/gems/.
#
# Please send email to atmtp-interest@sun.com with feedback,
# questions, or to request future announcements about ATMTP.
#
# ----------------------------------------------------------------------
#
# ATMTP is distributed as part of the GEMS software toolset and is
# available for use and modification under the terms of version 2 of
# the GNU General Public License.  The GNU General Public License is
# contained in the file $GEMS/LICENSE.
#
# Multifacet GEMS is free software; you can redistribute it and/or
# modify it under the terms of version 2 of the GNU General Public
# License as published by the Free Software Foundation.
#
# Multifacet GEMS is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with the Multifacet GEMS; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307,
# USA
#
# ----------------------------------------------------------------------

TARGET        = cps-test

# Libraries and source files:

CPPSRCS       = cps-test.cpp sync.il
OBJS          = cps-test.o inst_test.o rock_tm.o

ASM_INL       = rock_tm.il sync.il

CC            = cc
CPP           = CC
ASM           = fbe
# Compilation flags:

# Allowed optimization levels: 1-5.
# --- DO NOT USE OPTIMIZATION LEVEL 0! ---

HTM_CFLAGS    = -O2 -m64 -mt         -Wc,-xarch=v9
HTM_CPPFLAGS  = -O2 -m64 -mt -Qoption cg -xarch=v9

DEBUG_FLAGS =  -xO1 -xs -g
TARGET_CFLAGS   = $(HTM_CFLAGS)
TARGET_CPPFLAGS = $(HTM_CPPFLAGS)

CFLAGS          = $(DEBUG_FLAGS) $(TARGET_CFLAGS)
CPPFLAGS        = $(DEBUG_FLAGS) $(TARGET_CPPFLAGS)

# Building Rules:

all: $(TARGET)


cps-test.o: cps-test.cpp
	$(CPP) $(CPPFLAGS) -c $(CPPSRCS)

inst_test.o: inst_test.il
	$(CPP) $(CPPFLAGS) -c inst_test.cpp inst_test.il

rock_tm.o: rock_tm.il rock_tm.cpp
	$(CPP) $(CPPFLAGS) -c rock_tm.cpp rock_tm.il

# Linkage rule:

$(TARGET): $(OBJS)
	$(CPP) $(CPPFLAGS) $(OBJS) -o $(TARGET) -lpthread -lposix4 -lsched

# General purpose rules:

printVersion:
	@$(CPP) -V

clean:
	rm -f $(OBJS) $(TARGET) $(DEPS)

rebuild: clean all

# Dependencies:

$(OBJS): $(ASM_INL)
