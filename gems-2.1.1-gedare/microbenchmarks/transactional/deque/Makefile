XACT_DIR   = ../common
INC        = -I$(XACT_DIR)

include $(XACT_DIR)/Common.Makedefs

EXEC_TM       = deque_TM
EXEC_LOCK     = deque_Lock  

all: $(EXEC_TM) $(EXEC_LOCK)

$(EXEC_TM): deque_TM.o transaction.o $(XACT_DIR)/transaction.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(EXEC_TM) deque_TM.o transaction.o 

$(EXEC_LOCK): deque_Lock.o transaction.o $(XACT_DIR)/transaction.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(EXEC_LOCK) deque_Lock.o transaction.o 

deque_TM.o: deque.c $(XACT_DIR)/transaction.h
	$(CC) $(CFLAGS) -c -o deque_TM.o deque.c

deque_Lock.o: deque.c $(XACT_DIR)/transaction.h
	$(CC) $(CFLAGS) -DGLOBAL_LOCK -c -o deque_Lock.o deque.c

transaction.o: $(XACT_DIR)/transaction.c $(XACT_DIR)/transaction.h
	$(CC) $(CFLAGS) -c -o transaction.o $(XACT_DIR)/transaction.c 
    
clean:
	rm -f $(EXEC_TM) $(EXEC_LOCK) *.o 

.c.o:
	$(CC) $(CFLAGS) -c $*.c
