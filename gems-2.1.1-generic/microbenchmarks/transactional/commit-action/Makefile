XACT_DIR  = ../common
INC       = -I$(XACT_DIR)

include $(XACT_DIR)/Common.Makedefs

all: commit-action 

commit-action: commit-action.o transaction.o $(XACT_DIR)/transaction.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o commit-action commit-action.o transaction.o
    
transaction.o: $(XACT_DIR)/transaction.c $(XACT_DIR)/transaction.h
	$(CC) $(CFLAGS) -c -o transaction.o $(XACT_DIR)/transaction.c 

clean:
	/bin/rm -f commit-action *.o *~
.c.o:
	$(CC) $(CFLAGS) -c $*.c
    
