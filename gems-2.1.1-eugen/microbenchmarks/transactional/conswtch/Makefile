CC          = gcc 
OPT_FLAGS   = -Wa,-xarch=v8plusa -DSIMICS
LD_FLAGS    = -lpthread
XACT_DIR    = ../common
INCL        = -I$(XACT_DIR)

all: conswtch 

conswtch: conswtch.o transaction.o $(XACT_DIR)/transaction.h
	$(CC) $(OPT_FLAGS) $(LD_FLAGS) -lrt -o conswtch conswtch.o transaction.o 

transaction.o: $(XACT_DIR)/transaction.c $(XACT_DIR)/transaction.h
	$(CC) -O3 $(OPT_FLAGS) $(INCL) -c -o transaction.o $(XACT_DIR)/transaction.c 
    
clean:
	/bin/rm -f *.o *~

.c.o:
	$(CC) $(OPT_FLAGS) $(INCL) -c $*.c
