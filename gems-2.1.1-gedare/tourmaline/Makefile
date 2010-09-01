#
# $Id: Makefile 1.0 05/11/02 11:52:12-05:00 gibson@mustardseed.cs.wisc.edu $
#

# Based HEAVILY on Ruby's Makefil

.PHONY: all module clean

# GEMS_ROOT points to the base of the CVS tree
GEMS_ROOT := ..

GENERATED_DIR := generated

# Note that VPATH has to be defined before the common Makefile is
# included.
VPATH = .:$(GEMS_ROOT)/common:$(GEMS_ROOT)/common/gzstream:profiler:transcon:common:config:simics:$(GENERATED_DIR):$(GEMS_ROOT)/common/ioutil

# Need to keep this statement here, the order is tricky here
include $(GEMS_ROOT)/common/Makefile.common

# shared code (../common) need to differentiate between ruby, opal, and tourmaline
SPECIAL_FLAGS += -DIS_TOURMALINE

# Flag to find the multitude of scattered headers
INCLUDES  = -I$(SIMICS_ROOT)/$(HOST_TYPE)/obj/include/
INCLUDES += -I$(SIMICS_ROOT)/$(HOST_TYPE)/obj/include/simics
INCLUDES += -I$(SIMICS_ROOT)/$(HOST_TYPE)/config

SPECIAL_FLAGS += -DTARGET_SPARC_V9 -DTARGET_VA_BITS=64 -DTARGET_PA_BITS=64

# all the Tourmaline sources 

SRC_C := \
	attrparse.c \
	attrlex.c

SRC_CPP := \
    initvar.C \
	confio.C \
	init.C \
	Tourmaline_Global.C \
	MemoryAccess.C \
	TourmalineConfig.C \
	Histogram.C \
	commands.C \
	interface.C \
	Profiler.C \
	TransactionControl.C \
	Serializer.C \
	GenericController.C


#########
#begin targets
#########

all:
ifndef DESTINATION
	@echo "Tourmaline Usage: make [-j #] DESTINATION=[wheremodulegoes]"
	exit 1
endif
	@echo "Tourmaline: Making the parser..."
	$(MAKE) $(GENERATED_DIR)/generated
	@echo "Tourmaline: Making self..."
	$(MAKE) objectfiles
	$(MAKE) module_go
	@echo "Tourmaline: Linking simics module..."
	cd $(SIMICS_ROOT)/$(HOST_TYPE)/lib; $(MAKE) tourmaline
	@echo "Tourmaline: Moving simics module..."
	$(MAKE) BUILD_FLAGS=-DSIMICS movemodule
	@echo "Tourmaline: my work is done here..."

clean: 
	@echo "Tourmaline: cleaning all..."
	rm -rf $(HOST_TYPE)
	rm -rf $(GENERATED_DIR) 
	rm -f core *~ */*~ 
	
$(BIN_DIR)/tourmaline.closure: simics_api_dummy.c $(OBJ) $(BIN_DIR)/created $(GENERATED_DIR)/generated
	$(CC) -o /dev/null $(LDFLAGS) simics/simics_api_dummy.c $(OBJ) 
	touch $@
	@echo "Template closure done"
	
$(GENERATED_DIR)/default_param.h: ../common/ioutil/embedtext.py config/tourmaline.defaults
	python ../common/ioutil/embedtext.py config/tourmaline.defaults $(GENERATED_DIR)/default_param.h global_default_param
	
module_go: $(GENERATED_DIR)/generated $(BIN_DIR)/tourmaline.closure
	@echo "Made module"

objectfiles: $(OBJ)

# Generate protocol handlers from protocol descriptions
$(GENERATED_DIR)/generated: $(GENERATED_DIR)/created attrparse.y attrlex.l
	@echo "Generating config parser"
	$(YACC) $(YFLAGS) -p at -o $(GENERATED_DIR)/attrparse.c ../common/ioutil/attrparse.y
	$(LEX) $(LFLAGS) -Pat -o$(GENERATED_DIR)/attrlex.c ../common/ioutil/attrlex.l
	touch $(GENERATED_DIR)/generated

movemodule:
	$(GEMS_ROOT)/scripts/prepare_simics_home.sh $(SIMICS_ROOT)/home/$(DESTINATION) $(HOST_TYPE)
	$(MV) $(SIMICS_ROOT)/$(HOST_TYPE)/lib/tourmaline.so $(SIMICS_ROOT)/home/$(DESTINATION)/modules/tourmaline-v9.so
	$(MV) $(SIMICS_ROOT)/$(HOST_TYPE)/lib/python/mod_tourmaline_commands.py $(SIMICS_ROOT)/home/$(DESTINATION)/modules/python

DEP := $(addprefix $(OBJ_DIR)/,$(SRC_C:.c=.d) $(SRC_CPP:.C=.d))

include $(GEMS_ROOT)/common/Makefile.dep

