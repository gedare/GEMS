XACT_DIR   = ../common
INC        = -I$(XACT_DIR)

include $(XACT_DIR)/Common.Makedefs

EXEC_TM    = contention_TM
EXEC_LOCK  = contention_Lock

all: $(EXEC_TM) $(EXEC_LOCK) 

$(EXEC_TM): contention_TM.o transaction.o $(XACT_DIR)/transaction.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(EXEC_TM) contention_TM.o transaction.o 

$(EXEC_LOCK): contention_Lock.o transaction.o $(XACT_DIR)/transaction.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(EXEC_LOCK) contention_Lock.o transaction.o 

contention_TM.o: contention.c $(XACT_DIR)/transaction.h
	$(CC) $(CFLAGS) -c -o contention_TM.o contention.c

contention_Lock.o: contention.c $(XACT_DIR)/transaction.h
	$(CC) $(CFLAGS) -DGLOBAL_LOCK -c -o contention_Lock.o contention.c

transaction.o: $(XACT_DIR)/transaction.c $(XACT_DIR)/transaction.h
	$(CC) $(CFLAGS) -c -o transaction.o $(XACT_DIR)/transaction.c 
    
clean:
	/bin/rm -f $(EXEC_TM) $(EXEC_LOCK) *.o *~

.c.o:
	$(CC) $(CFLAGS) -c $*.c
