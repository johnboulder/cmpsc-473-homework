CC=gcc
CFLAGS=-Wall -g -std=gnu99
#TARGETS=project2
OBJFILES=psumemory.o ${t}.o

all: psumemory.o ${t}.o proj clean

psumemory.o: psumemory.c
	$(CC) -c $^ $(CFLAGS)
	
${t}.o: ${t}.c
	$(CC) -c $^ $(CFLAGS)

proj: ${t}.o psumemory.o
	$(CC) -o $@ $^ $(CFLAGS)

# Cleanup 
clean:
	rm -f $(OBJFILES) 
  
