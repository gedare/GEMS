XACT_DIR    = ../common
INC        = -I$(XACT_DIR)

include $(XACT_DIR)/Common.Makedefs

EXEC_TM = slist_TM
EXEC_LOCK = slist_Lock

all: $(EXEC_TM) $(EXEC_LOCK)

$(EXEC_TM): slist_TM.o transaction.o $(XACT_DIR)/transaction.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(EXEC_TM) slist_TM.o transaction.o

$(EXEC_LOCK): slist_Lock.o transaction.o $(XACT_DIR)/transaction.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(EXEC_LOCK) slist_Lock.o transaction.o

slist_TM.o: sortedList.c $(XACT_DIR)/transaction.h
	$(CC) $(CFLAGS) -c -o slist_TM.o sortedList.c

slist_Lock.o: sortedList.c $(XACT_DIR)/transaction.h
	$(CC) $(CFLAGS) -DLOCK -c -o slist_Lock.o sortedList.c

transaction.o: $(XACT_DIR)/transaction.c $(XACT_DIR)/transaction.h
	$(CC) $(CFLAGS) -c -o transaction.o $(XACT_DIR)/transaction.c 
    
clean:
	/bin/rm -f $(EXEC_TM) $(EXEC_LOCK) *.o 

.c.o:
	$(CC) $(CFLAGS) -c $*.c

