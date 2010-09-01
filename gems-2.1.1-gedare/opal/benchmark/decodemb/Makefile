
ASFLAGS = -xarch=v8plusa

# -s strips executable
# -e defines entry point
dmb: dmb.o
	ld -e main -o dmb dmb.o

clean:
	-rm -rf *.o *core hw
