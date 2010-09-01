XACT_DIR    = ../common
INC        = -I$(XACT_DIR)

include $(XACT_DIR)/Common.Makedefs

EXEC_TM = prioqueue_TM
EXEC_LOCK = prioqueue_Lock

all: $(EXEC_TM) $(EXEC_LOCK)

$(EXEC_TM): prioqueue_TM.o transaction.o $(XACT_DIR)/transaction.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(EXEC_TM) prioqueue_TM.o transaction.o

$(EXEC_LOCK): prioqueue_Lock.o transaction.o $(XACT_DIR)/transaction.h
	$(CC) $(CFLAGS) -DGLOBAL_LOCK $(LDFLAGS) -o $(EXEC_LOCK) prioqueue_Lock.o transaction.o

prioqueue_TM.o: prioqueue.c $(XACT_DIR)/transaction.h
	$(CC) $(CFLAGS) -c -o prioqueue_TM.o prioqueue.c

prioqueue_Lock.o: prioqueue.c $(XACT_DIR)/transaction.h
	$(CC) $(CFLAGS) -DGLOBAL_LOCK -c -o prioqueue_Lock.o prioqueue.c

transaction.o: $(XACT_DIR)/transaction.c $(XACT_DIR)/transaction.h
	$(CC) $(CFLAGS) -c -o transaction.o $(XACT_DIR)/transaction.c 
    
clean:
	/bin/rm -f $(EXEC_TM) $(EXEC_LOCK) *.o 

.c.o:
	$(CC) $(CFLAGS) -c $*.c
