XACT_DIR    = ../common
INC         = -I$(XACT_DIR)
EXEC				= logging-test 

include $(XACT_DIR)/Common.Makedefs

all: $(EXEC)

logging-test: logging-test.o transaction.o
	$(CC) $(CFLAGS) -o logging-test logging-test.o transaction.o

logging-test.o: logging-test.c logging-test.h
	$(CC) $(CFLAGS) -c logging-test.c

transaction.o: $(XACT_DIR)/transaction.c $(XACT_DIR)/transaction.h
	$(CC) $(CFLAGS) -c $(XACT_DIR)/transaction.c 

clean:
	/bin/rm -f *.s *.o *~ $(EXEC) core*
.c.o:
	$(CC) $(CFLAGS) -c $*.c
