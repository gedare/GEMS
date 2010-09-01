XACT_DIR  = ../common
INC       = -I$(XACT_DIR)

include $(XACT_DIR)/Common.Makedefs

all: partial-rollback

partial-rollback: partial-rollback.o transaction.o $(XACT_DIR)/transaction.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o partial-rollback partial-rollback.o transaction.o
    
transaction.o: $(XACT_DIR)/transaction.c $(XACT_DIR)/transaction.h
	$(CC) $(CFLAGS) -c -o transaction.o $(XACT_DIR)/transaction.c 

clean:
	/bin/rm -f partial-rollback *.o *~
.c.o:
	$(CC) $(CFLAGS) -c $*.c
    
