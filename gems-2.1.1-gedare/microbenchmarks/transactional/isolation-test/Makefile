XACT_DIR    = ../common
INC         = -I$(XACT_DIR)
EXEC				= isolation-test 

include $(XACT_DIR)/Common.Makedefs

all: $(EXEC)

isolation-test: isolation-test.o transaction.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o isolation-test isolation-test.o transaction.o

transaction.o: $(XACT_DIR)/transaction.c $(XACT_DIR)/transaction.h
	$(CC) $(CFLAGS) -c $(XACT_DIR)/transaction.c 

clean:
	/bin/rm -f *.s *.o *~ core* $(EXEC)
.c.o:
	$(CC) $(CFLAGS) -c $*.c
