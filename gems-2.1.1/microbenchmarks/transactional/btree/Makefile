
XACT_DIR = ../common
INC = -I$(XACT_DIR)

include $(XACT_DIR)/Common.Makedefs

# 1 freelist
SINGLE_FREELIST = 1
BTREE_TRANS_SFL = BTREE_TM_$(SINGLE_FREELIST)
# Multiple freelists
MULTIPLE_FREELIST = 31
BTREE_TRANS_MFL = BTREE_TM_$(MULTIPLE_FREELIST)

LARGE_NODE_SIZE = 1022
SMALL_NODE_SIZE = 9

EXTRAFLAGS = 

all: $(BTREE_TRANS_SFL) $(BTREE_TRANS_MFL) $(BTREE_LG_TRANS) $(BTREE_MULTI) 

$(BTREE_TRANS_SFL): btree_c.h btree_c_util.h btree_trans.c btree_c.o btree_c.h btree_c_util.o transaction.o btree_c_main.c
	$(CC) btree_trans.c btree_c.o btree_c_util.o transaction.o btree_c_main.c $(CFLAGS) -o $(BTREE_TRANS_SFL) $(LDFLAGS) $(EXTRAFLAGS) -DNODE_NUM_PAIRS=$(SMALL_NODE_SIZE) -DNUM_POOLS=$(SINGLE_FREELIST)

$(BTREE_TRANS_MFL): btree_c.h btree_c_util.h btree_trans.c btree_c_multiple_freelist.o btree_c.h btree_c_util.o transaction.o btree_c_main.c
	$(CC) btree_trans.c btree_c_multiple_freelist.o btree_c_util.o transaction.o btree_c_main.c $(CFLAGS) -o $(BTREE_TRANS_MFL) $(LDFLAGS) $(EXTRAFLAGS) -DNODE_NUM_PAIRS=$(SMALL_NODE_SIZE) -DNUM_POOLS=$(MULTIPLE_FREELIST)

$(BTREE_LG_TRANS): btree_c.h btree_trans.c btree_c_lg.o btree_c.h btree_c_util_lg.o transaction.o btree_c_main.c
	$(CC) btree_trans.c btree_c_lg.o btree_c_util_lg.o transaction.o btree_c_main.c $(CFLAGS) -o $(BTREE_LG_TRANS) $(LDFLAGS) $(EXTRAFLAGS) -DNODE_NUM_PAIRS=$(LARGE_NODE_SIZE) -DNUM_POOLS=$(SINGLE_FREELIST)

#--------------------------------------------

btree_c_lg.o: btree_c.c btree_c.h
	$(CC) -c $(CFLAGS) -o btree_c_lg.o -DNODE_NUM_PAIRS=$(LARGE_NODE_SIZE) -DNUM_POOLS=$(SINGLE_FREELIST) btree_c.c

btree_c_multiple_freelist.o: btree_c.c btree_c.h
	$(CC) -c $(CFLAGS) -o btree_c_multiple_freelist.o -DNODE_NUM_PAIRS=$(SMALL_NODE_SIZE) -DNUM_POOLS=$(MULTIPLE_FREELIST) btree_c.c

btree_c_util_lg.o: btree_c_util.c btree_c_util.h
	$(CC) -c $(CFLAGS) -o btree_c_util_lg.o -DNODE_NUM_PAIRS=$(LARGE_NODE_SIZE) -DNUM_POOLS=$(SINGLE_FREELIST) btree_c_util.c

transaction.o: ../common/transaction.c ../common/transaction.h
	$(CC) -c $(CFLAGS) -o transaction.o ../common/transaction.c 

#--------------------------------------------

.SUFFIXES:
.SUFFIXES:	.o .c .h

.c.o:
	$(CC) -c $(CFLAGS) $*.c -DNODE_NUM_PAIRS=$(SMALL_NODE_SIZE) -DNUM_POOLS=$(SINGLE_FREELIST)

clean:
	rm -f $(BTREE_TRANS_MFL) $(BTREE_TRANS_SFL) *.o 

