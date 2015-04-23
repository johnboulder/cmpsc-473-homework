// MAPPER STAGE
// PRODUCER: map-reader-i adds entries to buffer_read in the form (word, 1) for each word (buffer_read is bounded in size)
// CONSUMER: map-adder-i reads entries from buffer_read and adds them to buffer_write in the form of (word, count) (buffer_write is not bounded)
// Since buffer_write exists for each producer consumer pair (there will be entries in the respective buffer_writes that
// 	exist in other buffer_writes), the buffers produced will have to be summed. Hence the purpose of the the reducer stage
// REDUCER STAGE
// Single threaded operation. Only runs once, and only after the final map-adder-n has finished running

#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmpsc473mr.h"

#define STRMAX 25

int brSize;
int bwSize;
int bufferSize;
int replicas;
int replica;
int mapperDone;

int push(node_t *node, node_t *head, int *size);
int pushToBack(node_t *node, node_t *head, int *size);
node_t *popHead(node_t *head, int *size);
node_t *findMatch(node_t *head, char *wrd);

typedef struct __reader_arg_t
{
	char *filename;
	node_t *buffer_read;
	pthread_mutex_t *lock;
	pthread_cond_t *cond;
} reader_arg_t;

typedef struct __adder_arg_t
{
	node_t *buffer_read;
	node_t *buffer_write;
	pthread_mutex_t *lock;
	pthread_cond_t *cond;
	int *thread_return_count;
} adder_arg_t;

// Returns a pointer to the second buffer that was made in this function. i.e. buffer_write
node_t* mapper(int bMax, int reps, char *filename, int *thread_return_count, int rep)
{
	// 1. Initialize a lock pointer
	pthread_mutex_t *lock = malloc(sizeof(pthread_mutex_t));
	int rc = pthread_mutex_init(lock, NULL);
	assert(rc == 0);

	// 2. Initialize the condition variable
	pthread_cond_t *cond = malloc(sizeof(pthread_cond_t));
	rc = pthread_cond_init(cond, NULL);
	assert(rc == 0);

	// 2. Create a new node for buffer_read
	node_t *buffer_read = malloc(sizeof(node_t));
	brSize = 0;

	// 3. Create a new node for buffer_write
	node_t *buffer_write = malloc(sizeof(node_t));
	bwSize = 0;

	// Allocate an argument type for reader
	adder_arg_t *adder_arg = malloc(sizeof(adder_arg_t));
	
	bufferSize = bMax;
	replicas = reps;
	replica = rep;
	mapperDone = 0;

	// Init adder_arg
	adder_arg->buffer_read = buffer_read;
	adder_arg->buffer_write = buffer_write;
	adder_arg->lock = lock;
	adder_arg->cond = cond;
	adder_arg->thread_return_count = thread_return_count;

	// Allocate an argument type for adder
	reader_arg_t *reader_arg = malloc(sizeof(reader_arg_t));

	// Init reader_arg
	reader_arg->filename = filename;
	reader_arg->buffer_read = buffer_read;
	reader_arg->lock = lock;
	reader_arg->cond = cond;

	// 4. Spin a pthread for reader
	pthread_t reader;
	rc = pthread_create(&reader, NULL, &map_reader, ((void*)reader_arg));
	assert(rc == 0);
	printf("thread:%d reader created\n", rep);

	// 5. Spin a pthread for adder
	pthread_t adder;
	rc = pthread_create(&adder, NULL, &map_adder, ((void*)adder_arg));
	assert(rc == 0);
	printf("thread:%d adder created\n", rep);

	// 6. Return the head to buffer_write for storage in the lists array
	return buffer_write;
}

void reducer()
{

}

// 
void *map_reader(void *arg)
{
	reader_arg_t *reader = (reader_arg_t*)arg;
	
	char delimiters[2] = {'\n',' '};

	// Create a new file pointer so the various threads don't mess with pointer positions
	FILE *file = fopen(reader->filename, "r");
	
	if(!file)
	{
		printf("File failed to open\n");
		return NULL;
	}

	pthread_mutex_t *lock = reader->lock;
	pthread_cond_t *cond = reader->cond;
	
	node_t *buffer_read = reader->buffer_read;

	// Find the size of the file
	int rc = fseek(file, 0, SEEK_END);
	int fileSize = ftell(file);

	// Part 1. Read from file#######################################
	// Calculate the length of the segment we actually have to read
	int startByte = ((replica-1)*fileSize)/replicas;
	int endByte = (replica*fileSize)/replicas;
	int readBytes = endByte - startByte;

	// Create a dynamically sized array based on that length
	char *readArray = malloc(readBytes);

	// Use fseek to position the file pointer at the correct start position
	rc = fseek(file, startByte, SEEK_SET);
	assert(rc == 0);

	// Use fread to read however many bytes the segment is, at sizeof(char) (a character) per element
	rc = fread(readArray, sizeof(char), readBytes/sizeof(char), file);
	assert(rc == readBytes);

	fclose(file);

	char* word = strtok(readArray, delimiters);

	// Part 2. Take strings and add them to buffer_read#############
	// Loop until we've reached the end of the array
		// If we can acquire the lock, and if the list isn't full, add strings to the list
		// If not, we yield
	while(word)
	{
		// Check if the buffer is too full
		if(brSize>=bufferSize)
		{
			printf("thread: %d reader waiting for buffer\n", replica);
			// Buffer is too full, so we have to wait
			rc = pthread_cond_wait(cond, lock);
			assert(rc == 0);
		}
		
		// Create a node for the first word
		node_t *w = malloc(sizeof(node_t));
		w->word = malloc(sizeof(char)*STRMAX);
		strncpy(w->word, word, STRMAX);
		w->count = 1;

		//printf("word:%s in thread:%d \n", word, replica);

		pthread_mutex_lock(lock);
		// CRITICAL SECTION START****
	
		// Add word to buffer_read
		push(w, buffer_read, &brSize);
	
		// CRITICAL SECTION END******
		pthread_mutex_unlock(lock);

		// Signal that a node was added to the buffer
		pthread_cond_signal(cond);

		word = strtok(readArray, delimiters);
	}
	
	// Deallocate anything we made in this function
	// END OF map-reader: Free any memory that was associated with the arguments except the buffer_write pointer
	free(buffer_read);
	free(file);
	free(readArray);
	free(arg);

	arg = NULL;
	file = NULL;
	readArray = NULL;
	mapperDone = 1;

	// Signal that the mapper has finished everything just in case
	pthread_cond_signal(cond);

	return NULL;
}

// 
void *map_adder(void *arg)
{
	adder_arg_t *adder = (adder_arg_t*)arg;
	node_t *buffer_read = adder->buffer_read;
	node_t *buffer_write = adder->buffer_write;
	pthread_mutex_t *lock = adder->lock;
	pthread_cond_t *cond = adder->cond;
	int *thread_return_count = adder->thread_return_count;

	// While mapper is not done, or while buffer_read has items
	while(!mapperDone || brSize>0)
	{
		int rc = 0;
		// TODO potential deadlock if mapper stops producing just after adder consumes last element in buffer_read
		// Check if the buffer is too full
		if(brSize<0)
		{
			printf("thread: %d adder waiting for buffer\n", replica);
			// Nothing in the buffer to consume, so we have to wait
			rc = pthread_cond_wait(cond, lock);
			assert(rc == 0);
		}
		
		pthread_mutex_lock(lock);
		// CRITICAL SECTION START****
		
		// Pop the head of buffer_read
		node_t *w = popHead(buffer_read, &brSize);

		// CRITICAL SECTION END******
		pthread_mutex_unlock(lock);
		
		// The list is empty somehow and we reached the critical section in error
		// Possibly due reader ending
		if(!w)
		{
			// Continue to the next iteration of the loop
			continue;
		}

		node_t *match = findMatch(buffer_write, w->word);

		// If a match is found 
		if(match)
		{
			// Increment the occurences of the match
			match->count++;

			// Destroy the old pointer since it already exists in buffer_write
			free(w);
			w = NULL;
		}
		// buffer_write is either empty, or a match was not found
		else
		{
			// Add the node to buffer_write
			push(w, buffer_write, &bwSize);
		}
	}

	// END OF map-adder: Free any memory that was associated with the arguments except the buffer_write pointer
	free(buffer_read);
	free(arg);
	pthread_mutex_destroy(lock);
	pthread_cond_destroy(cond);
	buffer_read = NULL;
	arg = NULL;
	lock = NULL;
	cond = NULL;

	*thread_return_count++;

	return NULL;
}


// Adds a node to the head of the list
int push(node_t *node, node_t *head, int *size)
{
	node->next = head;
	head = node;
	*size++;
	return 0;
}

// Adds a node to the very back of the list
int pushToBack(node_t *node, node_t *head, int *size)
{
	node_t *current = head;
	node_t *prev = NULL;
	
	while(current)
	{
		prev = current;
		current = current->next;
	}

	if(prev)
	{
		prev->next = node;
	}
	else
	{
		push(node, head, size);
	}

	*size++;
	return 0;
}

// Removes a node from the head of the list
node_t *popHead(node_t *head, int *size)
{
	if(head)
	{
		node_t *node = head;
		head = head->next;
		*size--;
		return node;
	}
	else
		return NULL;
}

// Finds a node in the list with a word member that matches wrd
node_t *findMatch(node_t *head, char *wrd)
{
	if(!head)
		return NULL;

	if(!wrd)
	{
		return NULL;
	}

	node_t *current = head;

	while(current)
	{
		if(strcmp(current->word, wrd) == 0)
		{
			return current;
		}

		current = current->next;
	}

	// string not found
	return NULL;
}
