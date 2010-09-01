XACT_DIR  = ../common
INC       = -I$(XACT_DIR)

include $(XACT_DIR)/Common.Makedefs

all: compensation 

compensation: compensation.o transaction.o $(XACT_DIR)/transaction.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o compensation compensation.o transaction.o
    
transaction.o: $(XACT_DIR)/transaction.c $(XACT_DIR)/transaction.h
	$(CC) $(CFLAGS) -c -o transaction.o $(XACT_DIR)/transaction.c 

clean:
	/bin/rm -f compensation *.o *~
.c.o:
	$(CC) $(CFLAGS) -c $*.c
    
