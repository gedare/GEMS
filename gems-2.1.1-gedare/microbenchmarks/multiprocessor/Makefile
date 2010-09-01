
CC := gcc
CFLAGS := -O2
#CFLAGS := -g -DDEBUG
LIBS := -lpthread


all: parallel lock_contend uncontended_locks

%.o: %.C
	$(CC) $(CFLAGS) -o $@ -c $<

parallel: parallel.o
	$(CC) -o parallel parallel.o $(LIBS)

tryLock.o: tryLock.s
	$(CC) $(CFLAGS) -c -o tryLock.o tryLock.s

lock_contend: lock_contend.o tryLock.o
	$(CC) $(CFLAGS) -o lock_contend lock_contend.o tryLock.o $(LIBS)

uncontended_locks: uncontended_locks.o tryLock.o
	$(CC) $(CFLAGS) -o uncontended_locks uncontended_locks.o tryLock.o $(LIBS)

clean: 
	\rm *.o parallel lock_contend uncontended_locks
