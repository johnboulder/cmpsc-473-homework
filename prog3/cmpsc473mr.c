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

int bufferSize;
int replicas;
//int node_t** buffer_read;
//int node_t** buffer_write;

int push(node_t *node, node_t **head, int *size);
node_t *popHead(node_t **head, int *size);
node_t *findMatch(node_t **head, char *word, node_t **prev, int size);
void printlist(node_t **head);

typedef struct __reader_arg_t
{
	char *filename;
	node_t **buffer_read;
	pthread_mutex_t *lock;
	pthread_cond_t *cond;
	int replica;
	int *mapperDone;
	int *brSize;
} reader_arg_t;

typedef struct __adder_arg_t
{
	node_t **buffer_read;
	node_t **buffer_write;
	pthread_mutex_t *lock;
	pthread_cond_t *cond;
	int *thread_return_count;
	int replica;
	int *mapperDone;
	int *brSize;
	int *bwSize;
} adder_arg_t;

// Returns a pointer to the second buffer that was made in this function. i.e. buffer_write
int mapper(int bMax, int reps, char *filename, int *thread_return_count, int rep, node_t **buffer_read, node_t **buffer_write, int* bwSize)
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
	//node_t *buffer_read = NULL;
	//buffer_read = malloc(sizeof(node_t));
	//buffer_read->next = NULL;
	//buffer_read->count = 0;
	//strncpy(buffer_read->word, "banana", STRMAX);

	// 3. Create a new node for buffer_write
	//node_t *buffer_write = NULL;
	//buffer_write = malloc(sizeof(node_t));
	//buffer_write->next = NULL;
	//buffer_write->count = 0;
	//strncpy(buffer_write->word, "banana", STRMAX);

	// Allocate an argument type for reader
	adder_arg_t *adder_arg = malloc(sizeof(adder_arg_t));
	
	bufferSize = bMax;
	replicas = reps;

	int *mapperFlag = malloc(sizeof(int));
	*mapperFlag = 0;

	int *bufferReadSize = malloc(sizeof(int));
	*bufferReadSize = 0;

	// Init adder_arg
	adder_arg->buffer_read = buffer_read;
	adder_arg->buffer_write = buffer_write;
	adder_arg->lock = lock;
	adder_arg->cond = cond;
	adder_arg->thread_return_count = thread_return_count;
	adder_arg->replica = rep;
	adder_arg->mapperDone = mapperFlag;
	adder_arg->brSize = bufferReadSize;
	adder_arg->bwSize = bwSize;

	// Allocate an argument type for adder
	reader_arg_t *reader_arg = malloc(sizeof(reader_arg_t));

	// Init reader_arg
	reader_arg->filename = filename;
	reader_arg->buffer_read = buffer_read;
	reader_arg->lock = lock;
	reader_arg->cond = cond;
	reader_arg->replica = rep;
	reader_arg->mapperDone = mapperFlag;
	reader_arg->brSize = bufferReadSize;

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
	return 0;
}

void reducer(node_t** bufferList, node_t** finalList, int replicas, int **bw_size_array)
{
	int listsEmpty = 0;
	// Pop the first element from the first non-null list, and add it to the final buffer
	// Search the remaining lists for nodes with that same word, and increment the head of the final list
	while(!listsEmpty)
	{
		int dummySize = 100;
		int i = 0;

		// While buffer at i is NULL
		while(!bufferList[i] && i<replicas)
		{
			++i;
		}

		if(i == replicas)
		{
			// Lists are empty
			listsEmpty = 1;

			// Leave the while loop
			break;
		}
		// Pop the head from the first non-empty list
		node_t* word = popHead(&bufferList[i], &dummySize);

		// Add that node to the finalList of nodes
		push(word, finalList, &dummySize);

		for(int j = i; j<replicas; j++)
		{	
			node_t *match = NULL;
			node_t *previous = NULL;

			// Find a match for the head of finalList
			match = findMatch(&bufferList[j], (*finalList)->word, &previous, *(bw_size_array[j]));

			if(previous && match)
			{
				// Remove the matching node
				previous->next = match->next;

				// Increment the count on node from finalList
				(*finalList)->count += match->count;

				// Free the temporary node
				free(match);
				match = NULL;
				--((*bw_size_array[j]));
			}
			else if(match)
			{
				// The match was the head, so pop it
				node_t *temp = popHead(&bufferList[i], &dummySize);

				// Increment the count on node from finalList
				(*finalList)->count += temp->count;

				// Free the temporary node
				free(temp);
				temp = NULL;
				--((*bw_size_array[j]));
			}
			//previous;
		}

	}

	return;
}

// 
void *map_reader(void *arg)
{
	reader_arg_t *reader = (reader_arg_t*)arg;
	
	// Init local variables
	pthread_mutex_t *lock = reader->lock;
	pthread_cond_t *cond = reader->cond;
	node_t **buffer_read = reader->buffer_read;
	int *brSize = reader->brSize;
	int *mapperDone = reader->mapperDone;
	int replica = reader->replica;
	
	char delimiters[2] = {'\n',' '};

	// Create a new file pointer so the various threads don't mess with pointer positions
	FILE *file = fopen(reader->filename, "r");
	
	if(!file)
	{
		printf("File failed to open\n");
		return NULL;
	}

	// Find the size of the file
	int rc = fseek(file, 0, SEEK_END);
	int fileSize = ftell(file);

	// Part 1. Read from file#######################################
	// Calculate the length of the segment we actually have to read
	int startByte = ((replica-1)*fileSize)/replicas;
	int endByte = (replica*fileSize)/replicas;
	int readBytes = endByte - startByte;

	// Create a dynamically sized array based on that length
	char *readArray;
	readArray = malloc(readBytes*sizeof(char));
	// Use fseek to position the file pointer at the correct start position
	rc = fseek(file, startByte, SEEK_SET);
	assert(rc == 0);

	// Use fread to read however many bytes the segment is, at sizeof(char) (a character) per element
	rc = fread(readArray, sizeof(char), readBytes/sizeof(char), file);
	assert(rc == readBytes);

	fclose(file);

	char *word = malloc(STRMAX*sizeof(char));
	strncpy(word, "banana", STRMAX);
	
	word = strtok_r(readArray, delimiters, &readArray);

	// Part 2. Take strings and add them to buffer_read#############
	// Loop until we've reached the end of the array
		// If we can acquire the lock, and if the list isn't full, add strings to the list
		// If not, we yield
	while(word)
	{
		node_t *newNode = malloc(sizeof(node_t));
		strncpy(newNode->word, word, STRMAX);
		newNode->next = NULL;
		newNode->count = 1;

		pthread_mutex_lock(lock);
		// Check if the buffer is too full
		if(*brSize >= bufferSize)
		{
			//printf("thread: %d reader waiting for buffer\n", replica);
			// Buffer is too full, so we have to wait
			rc = pthread_cond_wait(cond, lock);
			assert(rc == 0);

			//printf("thread: %d reader continuing\n", replica);
		}
		// CRITICAL SECTION START****
		// Add word to buffer_read
		//printlist(buffer_read);

		push(newNode, buffer_read, brSize);

		//printlist(buffer_read);
	
		// CRITICAL SECTION END******
		pthread_mutex_unlock(lock);

		// Signal that a node was added to the buffer
		pthread_cond_broadcast(cond);

		word = NULL;

		word = strtok_r(readArray, delimiters, &readArray);
	}


	pthread_mutex_lock(lock);
	*mapperDone = 1;
	while(*mapperDone == 1)
	{
		pthread_cond_wait(cond, lock);
	}
	pthread_mutex_unlock(lock);
	// Deallocate anything we made in this function
	// END OF map-reader: Free any memory that was associated with the arguments except the buffer_write pointer
	// Set readArray back to its original position before we free it
	readArray-=readBytes;
	free(file);
	free(readArray);
	free(arg);
	free(word);

	file = NULL;
	readArray = NULL;
	arg = NULL;
	word = NULL;
	

	// Signal that the mapper has finished everything just in case
	pthread_cond_signal(cond);

	return NULL;
}

// 
void *map_adder(void *arg)
{
	adder_arg_t *adder = (adder_arg_t*)arg;
	node_t **buffer_read = adder->buffer_read;
	node_t **buffer_write = adder->buffer_write;
	pthread_mutex_t *lock = adder->lock;
	pthread_cond_t *cond = adder->cond;
	int *thread_return_count = adder->thread_return_count;
	int *brSize = adder->brSize;
	int *mapperDone = adder->mapperDone;
	int replica = adder->replica;
	int *bwSize = adder->bwSize;

	node_t *nothing = malloc(sizeof(node_t));
	
	char *word = malloc(STRMAX*sizeof(char));
	strncpy(word, "banana", STRMAX);

	// While mapper is not done, or while buffer_read has items
	while(!(*mapperDone) || *brSize>0)
	{
		int rc = 0;
		// TODO potential deadlock if mapper stops producing just after adder consumes last element in buffer_read
		pthread_mutex_lock(lock);
		// Check if the buffer is too full
		if(*brSize<=0)
		{
			//printf("thread: %d adder waiting for buffer\n", replica);
			// Nothing in the buffer to consume, so we have to wait
			rc = pthread_cond_wait(cond, lock);
			assert(rc == 0);
			//printf("thread: %d adder continuing\n", replica);
		}
		// CRITICAL SECTION START****
		//printlist(buffer_read);

		// Pop the head of buffer_read
		node_t* tempNode = popHead(buffer_read, brSize);

		//printlist(buffer_read);
		// CRITICAL SECTION END******
		pthread_mutex_unlock(lock);
		
		//TODO make sure this is actually returning a reference to a pointer
		node_t *match = findMatch(buffer_write, tempNode->word, &nothing, *bwSize);

		// If a match is found 
		if(match != NULL)
		{
			// Increment the occurences of the match
			match->count++;

			// Destroy the old pointer since it already exists in buffer_write
			free(tempNode);
			tempNode = NULL;
		}
		// buffer_write is either empty, or a match was not found
		else
		{
			// Add the node to buffer_write
			push(tempNode, buffer_write, bwSize);
		}
		pthread_cond_broadcast(cond);
	}

	pthread_mutex_lock(lock);
	if(*mapperDone == 1)
	{
		*mapperDone = 0;
	}
	pthread_mutex_unlock(lock);

	pthread_cond_broadcast(cond);
	// END OF map-adder: Free any memory that was associated with the arguments except the buffer_write pointer
	free(arg);
	free(word);
	free(nothing);
	pthread_mutex_destroy(lock);
	pthread_cond_destroy(cond);
	
	arg = NULL;
	word = NULL;
	nothing = NULL;
	lock = NULL;
	cond = NULL;
	++(*thread_return_count);

	return NULL;
}


// Adds a node to the head of the list
int push(node_t* node, node_t **head, int *size)
{
	if(!(node))
	{
		return -1;
	}
	
	node->next = *head;
	*head = node;
	++(*size);

	return 0;
}

// Removes a node from the head of the list
node_t *popHead(node_t **head, int *size)
{
	if((*head))
	{
		node_t *temp = (*head);
		*head = temp->next;
		temp->next = NULL;

		--(*size);
		//printf("word: %s\n", temp->word);
		return temp;
	}

	else
		return NULL;
}

// Finds a node in the list with a word member that matches wrd
node_t *findMatch(node_t **head, char *word, node_t **prev, int size)
{
	if(!(*head))
		return NULL;

	if(!word)
	{
		return NULL;
	}

	node_t *current = (*head);
	//*prev = NULL;
	int i = 0;
	while(current && i < size)
	{
		if(strcmp(current->word, word) == 0)
		{
			return current;
		}
		
		*prev = current;
		current = current->next;
		i++;
	}

	// string not found
	//*prev = NULL;
	return NULL;
}

void printlist(node_t **head)
{
	printf("\n\n**********\n");
	node_t *current = *head;

	while(current)
	{
		printf("word: %s\n", current->word);
		current = current->next;
	}
	printf("**********\n\n");
}
