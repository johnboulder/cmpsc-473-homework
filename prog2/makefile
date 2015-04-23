CC=gcc
CFLAGS=-Wall -fpic -std=gnu99
#TARGETS=project2
OBJFILES=psumemory.o ${t}.o

all: psumemory.o

psumemory.o: psumemory.c
	$(CC) -c $^ $(CFLAGS)
	
${t}.o: ${t}.c
	$(CC) -c $^ $(CFLAGS)

proj: ${t}.o psumemory.o
	$(CC) -o $@ $^ $(CFLAGS)

# Cleanup 
clean:
	rm -f $(OBJFILES) 
  
