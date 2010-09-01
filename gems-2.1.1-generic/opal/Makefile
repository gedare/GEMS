# Makefile 
#
# $Id: Makefile 1.120 05/01/20 22:17:02-06:00 mikem@maya.cs.wisc.edu $
#

.PHONY: all module tester clean tags

# GEMS_ROOT points to the base of the CVS tree
GEMS_ROOT := ..
GENERATED_DIR := generated
VPATH := common:fetch:power:sparc:system:module:trace:tester:$(GEMS_ROOT)/common:$(GEMS_ROOT)/common/ioutil:$(GENERATED_DIR)

# DESTINATION = Your Destination Here

include $(GEMS_ROOT)/common/Makefile.common

#
# FLAGS
#


## Sun Native compiler: you must run configure in v9-sol8-32/64 directory with CC and CXX defined to be SUNWspro executables before enabling these options.
#CC = /opt/SUNWspro/bin/CC
#DEPEND_FLAGS := -xM1
#DEBUG_FLAGS += -DUSE_DIRENT -DNO_FSR -DSWAP_ENDIAN
#TESTER_LDFLAGS := -lCstd
#OPT_FLAGS := -g -c -xO0
OPT_FLAGS := -ggdb -g3 -O2 -fPIC -finline-functions
#OPT_FLAGS := -xCC  -fast -xarch=v8plusa -xO4 -ftrap=%all,no%inexact -xs -xprefetch=yes

# if compiling to run standalone with ruby, specify a protocol here
PROTOCOL= MOSI_bcast_2level
RUBY_DIR=$(GEMS_ROOT)/ruby/$(PROTOCOL)/$(HOST_TYPE)/tester/

# define is_opal for cross-compiling packages
DEBUG_FLAGS += -DIS_OPAL

#    My standard debug package
#DEBUG_FLAGS += -DMODINIT_VERBOSE -DVERIFY_SIMICS -DPIPELINE_VIS -DDEBUG_RETIRE -DDEBUG_LSQ -DDEBUG_FILTER -DDEBUG_TESTER

#    My standard "functional correctness" debugging package
#DEBUG_FLAGS  += -DDEBUG_FUNCTIONALITY

#    defines for startup debugging
#DEBUG_FLAGS += -DMODINIT_VERBOSE
#    some run-time checks to validate simics envr is what we expect
DEBUG_FLAGS += -DVERIFY_SIMICS
#    PIPELINE_VIS: define for pipeline vis stuff
#DEBUG_FLAGS += -DPIPELINE_VIS

#    DEBUG_RETIRE for more verbose retirement
#DEBUG_FLAGS += -DDEBUG_RETIRE
#    defines debugging of the LSQ
#DEBUG_FLAGS += -DDEBUG_LSQ
#    Enables enables a debug filtering mechanism based on cycle, per proc
DEBUG_FLAGS += -DDEBUG_FILTER
#DEBUG_FLAGS += -DPRINT_L2MISS

#DEBUG_FLAGS += -DDEBUG_REG
#DEBUG_FLAGS += -DDEBUG_DYNAMIC_RET

#    defines a lot of verbosity in pseq
#DEBUG_FLAGS += -DDEBUG_PSEQ
#    defines debugging of the ruby interface
#DEBUG_FLAGS += -DDEBUG_RUBY
#    defines a lot of verbosity in the cache
#DEBUG_FLAGS += -DDEBUG_CACHE
#    DEBUG_TESTER: prints out differences between simics steps
#                  may define for tester, not for other modules
#DEBUG_FLAGS += -DDEBUG_TESTER
#   DEBUG_CC to debug condition codes
#DEBUG_FLAGS += -DDEBUG_CC

#    Define to debug ideal (inorder) processor modes
#DEBUG_FLAGS += -DDEBUG_IDEAL

#    FAKE_RUBY allows opal to imitate running w/ruby, stand-alone or in simics.
#DEBUG_FLAGS += -DFAKE_RUBY

#    defines for "zero tolerance" retirement checking: if a single register
#    doesn't match -- reload the entire state && squash the pipeline
DEBUG_FLAGS += -DRETIRE_ZERO_TOLERANCE

#    define to decode static instructions on a per-dynamic instruction basis
#    for higher performance, leave this commented out
DEBUG_FLAGS += -DREDECODE_EACH

#    define to cache instruction TLB translations on a per processor basis
#    for higher performance, uncomment this define
#DEBUG_FLAGS += -DUSE_MINI_ITLB

#    define to create statistics structures that are expensive in runtime&space
#DEBUG_FLAGS += -DEXPENSIVE_STATS

#    define to track the number of renames per logical register
#DEBUG_FLAGS += -DRENAME_EXPERIMENT

#    define run-time register file leak checking. Computationally expensive!
#DEBUG_FLAGS += -DCHECK_REGFILE

#    EXPENSIVE_ASSERTIONS mainly cover verifying the LSQ, scheduler lists, etc
#DEBUG_FLAGS += -DEXPENSIVE_ASSERTIONS

#    defines for static instruction window profiling
#DEBUG_FLAGS += -DSTATICI_PROFILE

#   MEMTRACE_DEBUG for debugging memory trace statements
#DEBUG_FLAGS += -DMEMTRACE_DEBUG

#    defines locations in the code where HOST endianess matters for the
#    correctness of this program.
#DEBUG_FLAGS += -DENDIAN_MATTERS

#    USE_FSR     defines for using the FSR (linux vs solaris)
#    USE_DIRENT  defines for different directory scanning (linux vs solaris)
#    SWAP_ENDIAN defines for endianness (explicit currently)
#    INSTRUCTION_OVERWRITE tags selected debugging of i-cache changes

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
  # Very bad
  INCLUDE += UNKNOWN_SIMICS_VERSION
endif
endif

INCLUDES += -Ibenchmark/tester

# CM extra: must define TARGET_SPARC_V9 to use sparc_api.h
SPECIAL_FLAGS += -DTARGET_SPARC_V9
# CM extra: must define TARGET_ULTRA to successfully use sparc_api.h include...
SPECIAL_FLAGS += -DTARGET_ULTRA -DTARGET_VA_BITS=64 -DTARGET_PA_BITS=64
SPECIAL_FLAGS += -DYY_NO_UNPUT

# A pointer to the location of the opal.o object created by simics's makefile
OPAL_MODULE_OBJ = ../simics/$(HOST_TYPE)/obj/modules/opal/opal.o

#
# SOURCE
#

SRC_CPP := \
	initvar.C \
	symtrace.C \
	chain.C \
	sstat.C \
	ptrace.C \
	pseq.C \
	system.C \
	dx.C \
	ix.C \
	exec.C \
	opcode.C \
	arf.C \
	actor.C \
	agree.C \
	bitlib.C \
	bitfield.C \
	bitdist.C \
	branchfile.C \
	cache.C \
	confio.C \
	controlop.C \
	decode.C \
	debugio.C \
	dependence.C \
	dtlb.C \
	dynamic.C \
	fatpredict.C \
	fileio.C \
	flow.C \
	flatarf.C \
	gshare.C \
	igshare.C \
	hfa.C \
	histogram.C \
	iwindow.C \
	ipage.C \
	ipagemap.C \
	indirect.C \
	listalloc.C \
	lockstat.C \
	lsq.C \
	mlpredict.C \
	memop.C \
	memstat.C \
	memtrace.C \
	mshr.C \
	pipepool.C \
	pipestate.C \
	power.C \
	pstate.C \
	ras.C \
	regbox.C \
	regfile.C \
	regmap.C \
	regstate.C \
	rubycache.C \
	scheduler.C \
	statici.C \
	storeSet.C \
	sysstat.C \
	stopwatch.C \
	simdist12.C \
	threadstat.C \
	tlstack.C \
	tracefile.C \
	transaction.C \
	utimer.C \
	wait.C \
	yags.C \
	writebuffer.C

SRC_C 	:= \
	ccops.c \
	attrparse.c \
	attrlex.c

#
# BUILD RULES
#

# The default rule is make the opal module
default_rule: module

all: tester usd readipage makeipage regtest memscan module

module:
	@echo "Opal: building module"
	${MAKE} $(GENERATED_DIR)/generated
	${MAKE} BUILD_FLAGS=-DSIMICS module_go
	@echo "Opal: Linking simics module"
ifeq ($(SIMICS_VERSION),2.2.X)
	cd $(SIMICS_ROOT)/$(HOST_TYPE)/lib; $(MAKE) opal
else
ifeq ($(SIMICS_VERSION),3.0)
	cd $(SIMICS_EXEC_ROOT); $(MAKE) opal
else
	@echo "Unrecognized value for SIMICS_VERSION: see commmon/Makefile.common"
	exit 1
endif
endif
ifdef DESTINATION
	@echo "Opal: Moving module"
	$(MAKE) BUILD_FLAGS=-DSIMICS movemodule DESTINATION=$(DESTINATION)	
endif
	echo "Finished."

tester:
	@echo "Opal: Building tester.exec"
	$(MAKE) $(BIN_DIR)/tester.exec

unitest:
	@echo "Opal: Building unitest.exec"
	@echo "Opal: Building ruby"
	cd $(GEMS_ROOT)/ruby; ${MAKE} SPECIAL_FLAGS=-DLIBRUBYTEST PROTOCOL=$(PROTOCOL) librubytest
	@echo "Opal: Building tester.exec"
	${MAKE} $(GENERATED_DIR)/generated
ifeq ($(PURIFY),)
	@echo "Opal: building simics objects"
ifeq ($(SIMICS_VERSION),2.2.X)
	cd $(SIMICS_ROOT)/$(HOST_TYPE)/lib; $(MAKE) opal
else
ifeq ($(SIMICS_VERSION),3.0)
	cd $(SIMICS_EXEC_ROOT); $(MAKE) opal
else
	@echo "Unrecognized value for SIMICS_VERSION: see commmon/Makefile.common"
	exit 1
endif
endif
endif
	@echo "Opal: linking ruby and opal"
	$(MAKE) BUILD_FLAGS=-DUNITESTER $(BIN_DIR)/unitest.exec

usd:        $(BIN_DIR)/usd.exec
	@echo "Opal: built usd"
makeipage:  $(BIN_DIR)/makeipage.exec
	@echo "Opal: built makeipage"
bp:         $(BIN_DIR)/bp.exec
	@echo "Opal: built bp"
readipage:  $(BIN_DIR)/readipage.exec
	@echo "Opal: built readipage"
readtrace:  $(BIN_DIR)/readtrace.exec
	@echo "Opal: built readtrace"
pipetest: $(BIN_DIR)/pipetest.exec
	@echo "Opal: built pipetest"
regtest:    $(BIN_DIR)/regtest.exec
	@echo "Opal: built regtest"
memscan:    $(BIN_DIR)/memscan.exec
	@echo "Opal: built memscan"
conftest:   $(BIN_DIR)/conftest.exec
	@echo "Opal: built conftest"
rhtest:     $(BIN_DIR)/rhtest.exec
	@echo "Opal: built rhtest"
ptracetest:   $(BIN_DIR)/ptracetest.exec
	@echo "Opal: built ptrace test module"

module_go: $(GENERATED_DIR)/generated $(OBJ)
	@echo "Made module"

$(GENERATED_DIR)/generated: $(GENERATED_DIR)/created attrparse.y attrlex.l $(GENERATED_DIR)/default_param.h
	@echo "Generating config parser"
	$(YACC) $(YFLAGS) -p at -o $(GENERATED_DIR)/attrparse.c ../common/ioutil/attrparse.y
	$(LEX) $(LFLAGS) -Pat -o$(GENERATED_DIR)/attrlex.c ../common/ioutil/attrlex.l
	touch $(GENERATED_DIR)/generated

$(GENERATED_DIR)/default_param.h: ../common/ioutil/embedtext.py config/config.defaults
	python ../common/ioutil/embedtext.py config/config.defaults $(GENERATED_DIR)/default_param.h global_default_param

movemodule:
ifeq ($(SIMICS_VERSION),2.2.X)
	$(GEMS_ROOT)/scripts/prepare_simics_home.sh $(SIMICS_ROOT)/home/$(DESTINATION) $(HOST_TYPE)
	$(MV) $(SIMICS_ROOT)/$(HOST_TYPE)/lib/opal.so $(SIMICS_ROOT)/home/$(DESTINATION)/modules/opal-v9.so
	$(MV) -f $(SIMICS_ROOT)/$(HOST_TYPE)/lib/python/mod_opal_commands.py $(SIMICS_ROOT)/home/$(DESTINATION)/modules/python
else
ifeq ($(SIMICS_VERSION),3.0)
	$(GEMS_ROOT)/scripts/prepare_simics_home.sh $(SIMICS_EXEC_ROOT)/home/$(DESTINATION) $(HOST_TYPE)
else
	@echo "Unrecognized value for SIMICS_VERSION: see commmon/Makefile.common"
	exit 1
endif
endif

removemodule:
ifeq ($(SIMICS_VERSION),2.2.X)
	$(RM) -f $(SIMICS_ROOT)/home/$(DESTINATION)/modules/opal-v9.so
	$(RM) -f $(SIMICS_ROOT)/home/$(DESTINATION)/modules/python/mod_opal_commands.py
	$(RM) -f $(SIMICS_ROOT)/home/$(DESTINATION)/modules/python/mod_opal_commands.pyc
	$(RM) -f $(SIMICS_ROOT)/$(HOST_TYPE)/lib/opal-v9.so
	$(RM) -f $(SIMICS_ROOT)/$(HOST_TYPE)/lib/python/mod_opal_commands.py
	$(RM) -f $(SIMICS_ROOT)/$(HOST_TYPE)/lib/python/mod_opal_commands.pyc
else
	@echo "removemodule target not supported for this SIMICS_VERSION"
	exit 1
endif

# -- Generate Tester
$(BIN_DIR)/tester.exec: $(OBJ) $(GENERATED_DIR)/generated $(BIN_DIR)/created $(OBJ_DIR)/simmain.o
	$(PURIFY) $(CC) -o $@ $(OPT_FLAGS) $(OPAL_MODULE_OBJ) $(OBJ_DIR)/simmain.o $(OBJ) $(TESTER_LDFLAGS)
	@echo "Made tester"

# -- Generate Ruby-Opal Tester
$(BIN_DIR)/unitest.exec: $(OBJ) $(GENERATED_DIR)/generated $(BIN_DIR)/created $(OBJ_DIR)/simmain.o $(RUBY_DIR)/bin/librubytest.a
	$(PURIFY) $(CC) -o $@ $(CFLAGS) $(OPT_FLAGS) $(OPAL_MODULE_OBJ) $(OBJ_DIR)/simmain.o $(OBJ) -L$(RUBY_DIR)/bin/ -lrubytest $(TESTER_LDFLAGS)
	@echo "Made unitester"

# -- Generate the Ipage map constructor
$(BIN_DIR)/makeipage.exec: $(OBJ) $(GENERATED_DIR)/generated tester/makeipage.C $(OBJ_DIR)/makeipage.o $(BIN_DIR)/created $(MODULE_REST_OBJ)
	$(PURIFY) $(CC) -o $@ $(OPT_FLAGS) $(OPAL_MODULE_OBJ) $(OBJ_DIR)/makeipage.o $(OBJ) $(TESTER_LDFLAGS)

# -- Generate the Ipage map tester
$(BIN_DIR)/readipage.exec: $(OBJ) $(GENERATED_DIR)/generated tester/readipage.C $(OBJ_DIR)/readipage.o $(BIN_DIR)/created $(MODULE_REST_OBJ)
	$(PURIFY) $(CC) -o $@ $(OPT_FLAGS) $(OPAL_MODULE_OBJ) $(OBJ_DIR)/readipage.o $(OBJ) $(TESTER_LDFLAGS)

# -- Generate the trace reader
$(BIN_DIR)/readtrace.exec: $(OBJ) $(GENERATED_DIR)/generated tester/readtrace.C $(OBJ_DIR)/readtrace.o $(BIN_DIR)/created $(MODULE_REST_OBJ)
	$(PURIFY) $(CC) -o $@ $(OPT_FLAGS) $(OPAL_MODULE_OBJ) $(OBJ_DIR)/readtrace.o $(OBJ) $(TESTER_LDFLAGS)

# -- Generate the UltraSparc Decoder
$(BIN_DIR)/usd.exec: $(OBJ) $(GENERATED_DIR)/generated tester/usd.C $(OBJ_DIR)/usd.o $(BIN_DIR)/created $(MODULE_REST_OBJ)
	$(PURIFY) $(CC) -o $@ $(OPT_FLAGS) $(OPAL_MODULE_OBJ) $(OBJ_DIR)/usd.o $(OBJ) $(TESTER_LDFLAGS)

# -- Generate the branch predictor executable
$(BIN_DIR)/bp.exec: $(OBJ) $(GENERATED_DIR)/generated tester/bp.C $(OBJ_DIR)/bp.o $(BIN_DIR)/created $(MODULE_REST_OBJ)
	$(PURIFY) $(CC) -o $@ $(OPT_FLAGS) $(OPAL_MODULE_OBJ) $(OBJ_DIR)/bp.o $(OBJ) $(TESTER_LDFLAGS)

# -- Generate the pipeline tester
$(BIN_DIR)/pipetest.exec: $(OBJ) $(GENERATED_DIR)/generated tester/pipetest.C $(OBJ_DIR)/pipetest.o $(BIN_DIR)/created $(MODULE_REST_OBJ)
	$(PURIFY) $(CC) -o $@ $(OPT_FLAGS) $(OPAL_MODULE_OBJ) $(OBJ_DIR)/pipetest.o $(OBJ) $(TESTER_LDFLAGS)

# -- Generate a register file tester
$(BIN_DIR)/regtest.exec: $(OBJ) $(GENERATED_DIR)/generated tester/regtest.C $(OBJ_DIR)/regtest.o $(BIN_DIR)/created $(MODULE_REST_OBJ)
	$(PURIFY) $(CC) -o $@ $(OPT_FLAGS) $(OPAL_MODULE_OBJ) $(OBJ_DIR)/regtest.o $(OBJ) $(TESTER_LDFLAGS)

# -- Generate a memory scan tester
$(BIN_DIR)/memscan.exec: $(OBJ) $(GENERATED_DIR)/generated tester/memscan.C $(OBJ_DIR)/memscan.o $(BIN_DIR)/created $(MODULE_REST_OBJ)
	$(PURIFY) $(CC) -o $@ $(OPT_FLAGS) $(OPAL_MODULE_OBJ) $(OBJ_DIR)/memscan.o $(OBJ) $(TESTER_LDFLAGS)

# -- Generate a configuration file tester
$(BIN_DIR)/conftest.exec: $(OBJ_DIR) $(ALLOBJ) $(OPAL_MODULE_OBJ) $(OBJ_DIR)/conftest.o $(BIN_DIR)/created $(MODULE_REST_OBJ)
	@echo "Making Opal module"
	$(PURIFY) $(CC) -o $@ $(OPT_FLAGS) $(OPAL_MODULE_OBJ) $(OBJ_DIR)/conftest.o $(OBJ) $(TESTER_LDFLAGS)

# -- Generate a ptrace file reader
$(BIN_DIR)/ptracetest.exec: $(OBJ_DIR) $(ALLOBJ) $(OPAL_MODULE_OBJ) $(OBJ_DIR)/ptracetest.o $(BIN_DIR)/created $(MODULE_REST_OBJ)
	@echo "Making Opal module"
	$(PURIFY) $(CC) -o $@ $(OPT_FLAGS) $(OPAL_MODULE_OBJ) $(OBJ_DIR)/ptracetest.o $(OBJ) $(TESTER_LDFLAGS)

# -- Generate run-ahead tester
$(BIN_DIR)/rhtest.exec: $(OBJ) $(GENERATED_DIR)/generated tester/rhtest.C $(OBJ_DIR)/rhtest.o $(BIN_DIR)/created $(MODULE_REST_OBJ)
	$(PURIFY) $(CC) -o $@ $(OPT_FLAGS) $(OPAL_MODULE_OBJ) $(OBJ_DIR)/rhtest.o $(OBJ) $(TESTER_LDFLAGS)

# -- Make a tar file of all the appropriate source files
tar:
	@echo "Building tar file"
	tar czvf opal.tgz Makefile common/*[Ch] simics/Makefile simics/*.[Chi] simics/*.py system/*.[Ccsh] simics/*.def system/*.def trace/*.[Ch] tester/*.[Ch] spitfire-mmu/*.[ch] where

doc:
# -d specifies subdirectory to write files into
# -g specifies to not generate GIFS
# -p specifies to document private member variables
	doc++ -g -p -d docs doc++.items

clean: 
ifeq ($(SIMICS_VERSION),2.2.X)
	/bin/rm -rf $(HOST_TYPE) $(GENERATED_DIR) ../simics/$(HOST_TYPE)/obj/extensions/opal
	$(RM) -rf $(SIMICS_ROOT)/home/$(DESTINATION)/modules/modcap.buf
else
ifeq ($(SIMICS_VERSION),3.0)
	$(RM) -rf $(HOST_TYPE) $(GENERATED_DIR)
else
	@echo "Unrecognized value for SIMICS_VERSION: see commmon/Makefile.common"
	exit 1
endif
endif

modclean:
ifeq ($(SIMICS_VERSION),2.2.X)
	/bin/rm -rf $(SIMICS_ROOT)/$(HOST_TYPE)/obj/modules/opal
else
ifeq ($(SIMICS_VERSION),3.0)
	$(RM) -rf $(SIMICS_EXEC_ROOT)/$(HOST_TYPE)/lib/opal*
else
	@echo "Unrecognized value for SIMICS_VERSION: see commmon/Makefile.common"
	exit 1
endif
endif

# rebuild for PIPELINE_VIS without doing a make clean
vis:
	/bin/rm $(OBJ_DIR)/controlop.o $(OBJ_DIR)/dynamic.o $(OBJ_DIR)/memop.o $(OBJ_DIR)/pseq.o $(OBJ_DIR)/debugio.o $(OBJ_DIR)/lsq.o

etags: 
	etags common/*.[Ch] system/*.[Ch] trace/*.[Ch] tester/*.[Ch]

include $(GEMS_ROOT)/common/Makefile.dep
