#
# This file has been modified by Kevin Moore and Dan Nussbaum of the
# Scalable Systems Research Group at Sun Microsystems Laboratories
# (http://research.sun.com/scalable/) to support the Adaptive
# Transactional Memory Test Platform (ATMTP).  For information about
# ATMTP, see the GEMS website: http://www.cs.wisc.edu/gems/.
#
#
# Please send email to atmtp-interest@sun.com with feedback,
# questions, or to request future announcements about ATMTP.
#
# For information about ATMTP, see the GEMS website:
# http://www.cs.wisc.edu/gems
#
# ----------------------------------------------------------------------
#
# File modification date: 2008-02-23
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

#
# $Id$
#

.PHONY: all module tester clean tags

# GEMS_ROOT points to the base of the CVS tree
GEMS_ROOT := ..

NETWORK = simple
GARNET = garnet-flexible-pipeline
GARNET_DETAIL = garnet-fixed-pipeline
ORION = orion

# Include the Makefile.include for the specific network. This
# needs to go before the VPATH, SPECIAL_FLAGS, or SRC_CPP are
# defined.

include network/${NETWORK}/Makefile.include
include network/${GARNET}/Makefile.include
include network/${GARNET_DETAIL}/Makefile.include
include network/${ORION}/Makefile.include

ifdef PROTOCOL
GENERATED_DIR := generated/$(PROTOCOL)
HTML_DIR := html/$(PROTOCOL)
endif

# Note that VPATH has to be defined before the common Makefile is
# included.
VPATH = ${NETWORK_VPATH}:${GARNET_VPATH}:.:${GARNET_DETAIL_VPATH}:${ORION_VPATH}:$(GEMS_ROOT)/common:$(GEMS_ROOT)/common/gzstream:buffers:common:slicc_interface:profiler:config:eventqueue:system:interfaces:simics:tester:recorder:log_tm:rock:$(GENERATED_DIR):$(GEMS_ROOT)/common/ioutil:network

# Need to keep this statement here, the order is tricky here
include $(GEMS_ROOT)/common/Makefile.common

ifdef NO_HTML
GENERATE_HTML := "no_html"
else
GENERATE_HTML := "html"
endif

ifdef PROTOCOL
# define dirs: overwrite BIN_DIR defined in Makefile.common
BIN_DIR := $(HOST_TYPE)/generated/$(PROTOCOL)/bin
RUBY_OBJ_DIR := $(HOST_TYPE)/generated/$(PROTOCOL)/obj
endif

# specify generated source and their dependencies
SRC_GENERATED := $(notdir $(shell ls $(GENERATED_DIR)/*.C))

# shared code (../common) need to differentiate between ruby and opal
SPECIAL_FLAGS += -DIS_RUBY

#DEBUG_FLAGS += -DRUBY_DEBUG=true
DEBUG_FLAGS += -DRUBY_DEBUG=false 

# Turn this ON to insert a layer of translation that will present
# Ruby with a contiguous memory space, even if Simics' actual memory space
# is non-contiguous
#SPECIAL_FLAGS += -DCONTIGUOUS_ADDRESSES

# enable coherence checking.
# See System::checkGlobalCoherence() for a more complete description of this
#DEBUG_FLAGS += -DCHECK_COHERENCE

### Enable a single level cache for the appropriate protocols

ifeq ($(SIMICS_VERSION),2.2.X) 
  INCLUDES  = -I$(SIMICS_ROOT)/$(HOST_TYPE)/obj/include/
  INCLUDES += -I$(SIMICS_ROOT)/$(HOST_TYPE)/obj/include/simics
  INCLUDES += -I$(SIMICS_ROOT)/$(HOST_TYPE)/config
else
ifeq ($(SIMICS_VERSION),3.0)
  INCLUDES  = -I$(SIMICS_INCLUDE_ROOT)
  INCLUDES += -I$(SIMICS_INCLUDE_ROOT)/simics
  INCLUDES += -I$(SIMICS_INCLUDE_ROOT)/simics/core
  INCLUDES += -I$(SIMICS_INCLUDE_ROOT)/simics/arch
  INCLUDES += -I$(SIMICS_INCLUDE_ROOT)/simics/util
else
  # This is really bad

endif
endif

ifeq ($(TARGET_MACHINE_TYPE), SPARC)
	SPECIAL_FLAGS += -DTARGET_SPARC_V9 -DTARGET_VA_BITS=64 -DTARGET_PA_BITS=64
else
	# for x86
	SPECIAL_FLAGS += -DTARGET_X86 -DTARGET_VA_BITS=64 -DTARGET_PA_BITS=64	
endif

ifdef PROTOCOL
DESC := $(shell cat $(PROTOCOL_ROOT)/RubySlicc_interfaces.slicc | grep -v "^\#") $(shell cat $(PROTOCOL_ROOT)/$(PROTOCOL).slicc | grep -v "^\#")
endif

# all the ruby sources 

SRC_C = \
	attrparse.c \
	attrlex.c

SRC_CPP := \
        $(SRC_GENERATED) \
	initvar.C \
	confio.C \
	main.C \
	test_framework.C \
	AbstractChip.C \
	AbstractCacheEntry.C \
	Address.C \
	DataBlock.C \
	Driver.C \
	SubBlock.C \
	Debug.C \
	AccessTraceForAddress.C \
	DirectoryMemory.C \
	EventQueue.C \
	EventQueueNode.C \
	Histogram.C \
	Message.C \
	MessageBuffer.C \
	MessageBufferNode.C \
	OpalInterface.C \
	Profiler.C \
	CacheProfiler.C \
	AddressProfiler.C \
	Sequencer.C \
	Set.C \
	NetDest.C \
	PersistentTable.C \
	NodePersistentTable.C \
	TimerTable.C \
	System.C \
	Global.C \
	init.C \
	util.C \
	gzstream.C \
	RubyConfig.C \
	SimicsDriver.C \
	SimicsProcessor.C \
	SimicsHypervisor.C \
	StoreBuffer.C \
	StoreCache.C \
	interface.C \
  RubySlicc_Profiler_interface.C \
	TraceRecord.C \
	Tracer.C \
	CacheRecorder.C \
	StoreTrace.C \
	commands.C \
	Check.C \
	CheckTable.C \
	Tester.C \
	RequestGenerator.C \
	SpecifiedGenerator.C \
	DetermGETXGenerator.C \
	DetermInvGenerator.C \
	DetermSeriesGETSGenerator.C \
	DeterministicDriver.C \
	Instruction.C \
	MemoryNode.C \
	MemoryControl.C \
	TransactionInterfaceManager.C \
	TransactionIsolationManager.C \
	TransactionVersionManager.C \
	TransactionConflictManager.C \
	LazyTransactionVersionManager.C \
  TransactionSimicsProcessor.C \
	PartialAddressFilter.C \
	RegisterStateWindowed.C \
	getopt.C \
	ContiguousAddressTranslator.C \
	GenericBloomFilter.C \
	LSB_CountingBloomFilter.C \
	NonCountingBloomFilter.C \
	BulkBloomFilter.C \
	BlockBloomFilter.C \
	MultiGrainBloomFilter.C \
	MultiBitSelBloomFilter.C \
	H3BloomFilter.C \
	XactIsolationChecker.C \
	XactCommitArbiter.C \
	XactProfiler.C \
	XactVisualizer.C \
	${NETWORK_SRC_CPP} \
	${GARNET_SRC_CPP} \
	${GARNET_DETAIL_SRC_CPP} \
	${ORION_SRC_CPP} \
	SyntheticDriver.C \
	Rock.C \
	RockTransactionManager.C \
	#XactAbortRequestGenerator.C \
	XactRequestGenerator.C \

#########
#begin targets
#########

RUBY_OBJ = $(addprefix $(RUBY_OBJ_DIR)/,$(SRC_C:.c=.o) $(SRC_CPP:.C=.o))
RUBY_DEP = $(addprefix $(RUBY_OBJ_DIR)/,$(SRC_C:.c=.d) $(SRC_CPP:.C=.d))

$(RUBY_OBJ_DIR)/%.o: %.C $(RUBY_OBJ_DIR)/created
	$(CC) -o $@ -c $< $(CFLAGS) 

$(RUBY_OBJ_DIR)/%.o: %.c $(RUBY_OBJ_DIR)/created
	$(CC) -o $@ -c $< $(CFLAGS)  

$(RUBY_OBJ_DIR)/%.d: %.C $(RUBY_OBJ_DIR)/created
	$(SHELL) -ec '$(CC) $(DEPEND_FLAGS) $(CFLAGS) $< | sed -e '\''s@^\(.*\)\.o:@$@:@'\'' | sed -e '\''s@^\(.*\)\.d:@\1.d \1.o:@'\'' > $@ #'

$(RUBY_OBJ_DIR)/%.d: %.c $(RUBY_OBJ_DIR)/created
	$(SHELL) -ec '$(CC) $(DEPEND_FLAGS) $(CFLAGS) $< | sed -e '\''s@^\(.*\)\.o:@$@:@'\'' | sed -e '\''s@^\(.*\)\.d:@\1.d \1.o:@'\'' > $@ #'


#.NOTPARALLEL: module tester

all:
ifndef PROTOCOL
	@echo "Ruby Usage: make [-j #] PROTOCOL=[protocolname]"
	@echo ""
	exit 1
endif
	@echo "Ruby: Building protocol:" $(PROTOCOL)
	@echo "Ruby: Making slicc"
	cd $(SLICC_ROOT); $(MAKE)
	@echo "Ruby: Running slicc"
	$(MAKE) $(GENERATED_DIR)/generated
	@echo "Ruby: Generating ruby objects"
	$(MAKE) $(BIN_DIR)/tester.exec
ifdef DESTINATION
	$(MAKE) module_go
	@echo "Ruby: Linking simics module"
ifeq ($(SIMICS_VERSION),2.2.X)
	cd $(SIMICS_ROOT)/$(HOST_TYPE)/lib; $(MAKE) ruby
else
ifeq ($(SIMICS_VERSION),3.0)
	cd $(SIMICS_EXEC_ROOT); $(MAKE) ruby
else
	@echo "Unrecognized value for SIMICS_VERSION: see common/Makefile.common"
	exit 1
endif
endif
	@echo "Ruby: Moving simics module"
	$(MAKE) BUILD_FLAGS=-DSIMICS movemodule
endif
ifdef TESTER_DESTINATION
	@echo "Ruby: Moving tester"
	mkdir -p $(GEMS_ROOT)/condor/bin
	$(MV) $(BIN_DIR)/tester.exec $(GEMS_ROOT)/condor/bin/$(TESTER_DESTINATION).exec
endif
	@echo "Ruby: my work is done here..."

module: all
	@echo "NOTE: Specifying 'module' or 'tester' is no longer required"

tester: all
	@echo "NOTE: Specifying 'module' or 'tester' is no longer required"

clean: 
	@echo "Ruby: cleaning all..."
	rm -rf $(HOST_TYPE)
	rm -rf html generated
	rm -f core *~ */*~ TAGS ruby.caches.gz ruby.trace.gz 
ifeq ($(HOST_TYPE),x86-linux)
ifeq ($(SIMICS_VERSION),2.2.X)
	rm -rf $(SIMICS_ROOT)/$(HOST_TYPE)/obj/extensions/ruby/
else
ifeq ($(SIMICS_VERSION),3.0)
	# do nothing
else
	@echo "Unrecognized value for SIMICS_VERSION: see common/Makefile.common"
	exit 1
endif
endif
endif

# Generate protocol handlers from protocol descriptions
$(GENERATED_DIR)/generated: $(GENERATED_DIR)/created $(HTML_DIR)/created $(DESC) $(SLICC) attrparse.y attrlex.l
	@echo "Generating protocol handlers"
	$(SLICC) $(GENERATED_DIR) $(HTML_DIR) $(PROTOCOL) $(GENERATE_HTML) $(DESC) 
	@echo "Generating config parser"
	$(YACC) $(YFLAGS) -p at -o $(GENERATED_DIR)/attrparse.c ../common/ioutil/attrparse.y
	$(LEX) $(LFLAGS) -Pat -o$(GENERATED_DIR)/attrlex.c ../common/ioutil/attrlex.l
	touch $(GENERATED_DIR)/generated

$(GENERATED_DIR)/default_param.h: ../common/ioutil/embedtext.py config/rubyconfig.defaults
	python ../common/ioutil/embedtext.py config/rubyconfig.defaults $(GENERATED_DIR)/default_param.h global_default_param

$(GENERATED_DIR)/tester_param.h: ../common/ioutil/embedtext.py config/tester.defaults
	python ../common/ioutil/embedtext.py config/tester.defaults $(GENERATED_DIR)/tester_param.h global_default_tester_param

$(BIN_DIR)/tester.exec: simics_api_dummy.c $(RUBY_OBJ) $(BIN_DIR)/created $(GENERATED_DIR)/generated
	@echo ""
	@echo "Linking tester binary ..."
	$(CC) -o $@ $(LDFLAGS) simics/simics_api_dummy.c $(RUBY_OBJ) 
	@echo "Made tester"

# make ruby.closure for template closure, as side effect some .o will get
# recompiled. If you see error in this stage, it might be the case you want to
# add more dummy functions in simics_api_dummy.c.
$(BIN_DIR)/ruby.closure: simics_api_dummy.c $(RUBY_OBJ) $(BIN_DIR)/created $(GENERATED_DIR)/generated
	$(CC) -o /dev/null $(LDFLAGS) simics/simics_api_dummy.c $(RUBY_OBJ) 
	touch $@
	@echo "Template closure done"

module_go: $(GENERATED_DIR)/generated $(BIN_DIR)/ruby.closure
	@echo "Made module"

movemodule:
ifeq ($(SIMICS_VERSION),2.2.X)
	$(GEMS_ROOT)/scripts/prepare_simics_home.sh $(SIMICS_ROOT)/home/$(DESTINATION) $(HOST_TYPE)
	$(MV) $(SIMICS_ROOT)/$(HOST_TYPE)/lib/ruby.so $(SIMICS_ROOT)/home/$(DESTINATION)/modules/ruby-v9.so
	$(MV) $(SIMICS_ROOT)/$(HOST_TYPE)/lib/python/mod_ruby_commands.py $(SIMICS_ROOT)/home/$(DESTINATION)/modules/python
else
ifeq ($(SIMICS_VERSION),3.0)
	$(GEMS_ROOT)/scripts/prepare_simics_home.sh $(SIMICS_EXEC_ROOT)/home/$(DESTINATION) $(HOST_TYPE)
else
	@echo "Unrecognized value for SIMICS_VERSION: see common/Makefile.common"
	exit 1
endif
endif


DEP := $(addprefix $(RUBY_OBJ_DIR)/,$(SRC_C:.c=.d) $(SRC_CPP:.C=.d))

include $(GEMS_ROOT)/common/Makefile.dep

