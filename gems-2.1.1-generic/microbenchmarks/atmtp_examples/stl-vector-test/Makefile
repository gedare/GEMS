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

HARNESS       = STLVector-harness.cpp

BASENAME        = stl-vectorFull

# Files that are needed to be compiled in two steps: .c{pp} --> .s --> .o , to
# workaround a compiler bug that regects calls the chkpt instructions:

REQASM	      =
REQASMCPP     = $(HARNESS)

REQASM_OBJS   = $(REQASM:.c=.o) $(REQASMCPP:.cpp=.o)
ASM_FILES     = $(REQASM:.c=.s) $(REQASMCPP:.cpp=.s)

# Source files:

SRCS          = rand-seq.c
CPPSRCS       = $(HARNESS) Locks.cpp rock_tm.cpp
OBJS          = $(SRCS:.c=.o) $(CPPSRCS:.cpp=.o)
DEPS          = $(SRCS:.c=.d) $(CPPSRCS:.cpp=.d)

ASM_INL       = sync.il rock_tm.il

COMPILER_DIR  = /opt/SUNWspro
CC            = $(COMPILER_DIR)/bin/cc
CPP           = $(COMPILER_DIR)/bin/CC
ASM           = $(COMPILER_DIR)/bin/fbe

# Compilation flags:

USER_INLINE=%auto,no%__1cQVectorThreadBody6Fpv_v_,no%inserting,no%deleting

NOROCKFP_FLAGS=-Qiselect-movxtod=0,-Qiselect-movitof=0,-Qiselect-unfused_muladd=0
NOINLINE_FLAGS=-xinline=sysCompareAndSwap,sysCompareAndSwap64,membarstoreload,$(USER_INLINE)

NOTM_CFLAGS   = -m64 -mt -DNO_HTM
HTM_CFLAGS    = -m64 -mt         -Wc,-xarch=v9

NOTM_CPPFLAGS = -m64 -mt -DNO_HTM
HTM_CPPFLAGS  = -m64 -mt -Qoption cg -xarch=v9

#Mode = Debug

ifeq    "$(Mode)" "Debug"
  DEBUG_FLAGS = -xO1 -xs -g -DHT_DEBUG -DDEBUG_PRINTFS
else  # "$(Mode)" != "Debug"
  DEBUG_FLAGS = -xO2
endif # "$(Mode)" != "Debug"

# To specify the compilation mode from the command line, define
# COMPMODE to be one of NOTM or HTM.
#
ifeq    "$(COMPMODE)"    "NOTM"
  TARGET          = $(BASENAME).noTM
  TARGET_CFLAGS   = $(NOTM_CFLAGS) -DTXLOCK_NO_ELISION
  TARGET_CPPFLAGS = $(NOTM_CPPFLAGS) -DTXLOCK_NO_ELISION
endif # "$(COMPMODE)" == "NOTM"
#
ifeq    "$(COMPMODE)"    "HTM"
TARGET 		= $(BASENAME).htm
TARGET_CFLAGS   = $(HTM_CFLAGS)
TARGET_CPPFLAGS = $(HTM_CPPFLAGS)
endif # "$(COMPMODE)" == "HTM"

ifdef  RWLOCKS
  TARGET_CFLAGS += -DUSE_RWLOCK
  TARGET_CPPFLAGS += -DUSE_RWLOCK
endif #RWLOCKS

#

TARGET_CPPFLAGS +=   -D_STLP_USE_NEWALLOC -library=stlport4
TARGET_CFLAGS +=   -D_STLP_USE_NEWALLOC -library=stlport4
TARGET_ASM_FLAGS  = -xarch=v9d

CFLAGS          = $(TARGET_CFLAGS)   $(TL2_FLAGS) $(NOINLINE_FLAGS) $(DEBUG_FLAGS)
CPPFLAGS        = $(TARGET_CPPFLAGS) $(TL2_FLAGS) $(NOINLINE_FLAGS) $(DEBUG_FLAGS)

# Building Rules:

all: $(TARGET)
$(REQASM_OBJS):%.o:%.s

%o:%s
	$(ASM) $(TARGET_ASM_FLAGS) $<
.c.o:
	@echo Regenerating $*.d
	@$(CPP) $(CFLAGS) -xM1 $< > $*.d
	$(CC) $(CFLAGS) -c $< $(ASM_INL)

.c.s:
	@echo Regenerating $*.d
	@$(CC) $(CFLAGS) -xM1 $< | sed 's/\.o\ :/.s :/' > $*.d
	$(CC) $(CFLAGS)    -S $< $(ASM_INL)

.cpp.s:
	@echo Regenerating $*.d
	@$(CPP) $(CPPFLAGS) -xM1 $< | sed 's/\.o\ :/.s :/' > $*.d
	$(CPP) $(CPPFLAGS) -S $< $(ASM_INL)

.cpp.o:
	@echo Regenerating $*.d
	@$(CPP) $(CPPFLAGS) -xM1 $< > $*.d
	$(CPP) $(CPPFLAGS) -c $< $(ASM_INL)

.cpp.p:
	$(CPP) $(CPPFLAGS) -E $< $(ASM_INL) > $*.p

.c.p:
	$(CC) $(CFLAGS) -E $< $(ASM_INL) > $*.p

# Linkage rule:

$(TARGET): $(OBJS)
	@echo "==========================================================="
	@echo "Linking..."
	@echo "==========================================================="
	$(CPP) $(CPPFLAGS) \
	    $(OBJS) \
	    -lpthread -lumem -lposix4 -lsched \
	    -o $(TARGET)

# General purpose rules:

printVersion:
	@$(CPP) -V

clean:
	rm -f $(OBJS) $(TARGET) $(DEPS) $(ASM_FILES)

rebuild: clean all

# Dependencies:

$(OBJS): $(ASM_INL)
-include $(DEPS)
