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

int push(node_t **node, node_t **head, int *size);
node_t *popHead(node_t **head, int *size);
node_t *findMatch(node_t **head, char *word, node_t **prev, int size);
void printlist(node_t **head);

/*typedef struct __node_t
{
	struct __node_t *next;
	char word[25];
	int count;
} node_t;*/

typedef struct __reader_arg_t
{
	pthread_mutex_t *lock;
	pthread_cond_t *cond;
	int replica;
	int *mapperDone;
	char *filename;
} reader_arg_t;

typedef struct __adder_arg_t
{
	pthread_mutex_t *lock;
	pthread_cond_t *cond;
	int replica;
	int *mapperDone;
} adder_arg_t;

/*##Variables##*/
node_t **buffer_reader_array;
node_t **buffer_adder_array;
int **bw_size_array;
int **br_size_array;
int replicas;
int bufferMaxSize;
int *threads_returned;
node_t *finalList;
int *flSize;


int main(int argc, void *argv[])
{
	clock_t begin, end;
	double time_spent;
	char *filename;

	if( argc == 4 )
	{
		filename = (char*)argv[1];
		replicas = atoi(argv[2]);
		bufferMaxSize = atoi(argv[3]);
	}
	else if( argc > 4 )
	{
		printf("Too many arguments. \n");
		return -1;
	}
	else
	{
		printf("Three arguments expected.\n");
		printf("[filename] [replicas] [buffer size]\n");
		return -1;
	}

	threads_returned = malloc(sizeof(int));
	*threads_returned = 0;
	finalList = NULL;

	br_size_array = malloc(sizeof(int*)*replicas);
	bw_size_array = malloc(sizeof(int*)*replicas);
	buffer_adder_array = malloc(sizeof(node_t*)*replicas);
	buffer_reader_array = malloc(sizeof(node_t*)*replicas);

	for(int i=1; i<=replicas; i++)
	{
		buffer_adder_array[i-1] = NULL;
		buffer_reader_array[i-1] = NULL;
		bw_size_array[i-1] = malloc(sizeof(int));
		*(bw_size_array[i-1]) = 0;
		br_size_array[i-1] = malloc(sizeof(int));
		*(br_size_array[i-1]) = 0;

		mapper(filename, i);
	}

	while(*threads_returned<replicas);

	reducer();

	for(int i = 0; i<replicas; i++)
	{
		free(bw_size_array[i]);
		free(br_size_array[i]);
	}

	// Free everything
	node_t *current = finalList;
	node_t *previous = NULL;

	while(current)
	{
		previous = current;
		current = current->next;
		free(previous);
	}

	free(bw_size_array);
	free(br_size_array);
	free(buffer_adder_array);
	free(buffer_reader_array);
	free(threads_returned);
	free(flSize);

	return 0;
}

// Returns a pointer to the second buffer that was made in this function. i.e. buffer_write
int mapper(char *filename, int rep)
{
	// 1. Initialize a lock pointer
	pthread_mutex_t *lock = malloc(sizeof(pthread_mutex_t));
	int rc = pthread_mutex_init(lock, NULL);
	assert(rc == 0);

	// 2. Initialize the condition variable
	pthread_cond_t *cond = malloc(sizeof(pthread_cond_t));
	rc = pthread_cond_init(cond, NULL);
	assert(rc == 0);

	// Allocate an argument type for reader
	adder_arg_t *adder_arg = malloc(sizeof(adder_arg_t));
	
	int *mapperFlag = malloc(sizeof(int));
	*mapperFlag = 0;

	// Init adder_arg
	adder_arg->lock = lock;
	adder_arg->cond = cond;
	adder_arg->replica = rep;
	adder_arg->mapperDone = mapperFlag;

	// Allocate an argument type for adder
	reader_arg_t *reader_arg = malloc(sizeof(reader_arg_t));

	// Init reader_arg
	reader_arg->lock = lock;
	reader_arg->cond = cond;
	reader_arg->replica = rep;
	reader_arg->mapperDone = mapperFlag;
	reader_arg->filename = filename;

	// 4. Spin a pthread for reader
	pthread_t reader;
	rc = pthread_create(&reader, NULL, &map_reader, ((void*)reader_arg));
	assert(rc == 0);
	//printf("thread:%d reader created\n", rep);

	// 5. Spin a pthread for adder
	pthread_t adder;
	rc = pthread_create(&adder, NULL, &map_adder, ((void*)adder_arg));
	assert(rc == 0);
	//printf("thread:%d adder created\n", rep);

	// 6. Return the head to buffer_write for storage in the lists array
	return 0;
}

void reducer()
{

	flSize = malloc(sizeof(int));
	*flSize = 0;
	int listsEmpty = 0;
	// Pop the first element from the first non-null list, and add it to the final buffer
	// Search the remaining lists for nodes with that same word, and increment the head of the final list
	while(!listsEmpty)
	{
		int i = 0;

		// While buffer at i is NULL
		while(!buffer_adder_array[i] && i<replicas)
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
		node_t* word = popHead(&buffer_adder_array[i], bw_size_array[i]);

		// Add that node to the finalList of nodes
		push(&word, &finalList, flSize);

		for(int j = i; j<replicas; j++)
		{	
			node_t *match = NULL;
			node_t *previous = NULL;

			// Find a match for the head of finalList
			match = findMatch(&(buffer_adder_array[j]), finalList->word, &previous, *(bw_size_array[j]));

			if(previous && match)
			{
				// Remove the matching node
				previous->next = match->next;

				// Increment the count on node from finalList
				finalList->count += match->count;

				// Free the temporary node
				free(match);
				match = NULL;
				--((*bw_size_array[j]));
			}
			else if(match)
			{
				// The match was the head, so pop it
				node_t *temp = popHead(&(buffer_adder_array[i]), bw_size_array[i]);

				if(!temp)
				{
					continue;
				}

				// Increment the count on node from finalList
				finalList->count += temp->count;

				// Free the temporary node
				free(temp);
				temp = NULL;
				--((*bw_size_array[j]));
			}
		}

	}
	
}

// 
void *map_reader(void *arg)
{	
	reader_arg_t *reader = (reader_arg_t*)arg;

	pthread_mutex_t *lock = reader->lock;
	pthread_cond_t *cond = reader->cond;
	int replica = reader->replica;
	int *mapperDone = reader->mapperDone;
	char *filename = reader->filename;
	char delimiters[2] = {'\n',' '};

	int *brSize = br_size_array[replica-1];
	node_t **buffer_read = &buffer_reader_array[replica-1];

	// Create a new file pointer so the various threads don't mess with pointer positions
	FILE *file = fopen(filename, "r");
	
	// Find the size of the file
	int rc = fseek(file, 0, SEEK_END);
	int fileSize = ftell(file);

	// Calculate the length of the segment we actually have to read
	int startByte = ((replica-1)*fileSize)/replicas;
	int endByte = (replica*fileSize)/replicas;
	int readBytes = endByte - startByte;

	// Create a dynamically sized array based on that length
	char *readArray;
	readArray = malloc(readBytes*sizeof(char));
	
	char* pointerToFree = readArray;
	
	// Use fseek to position the file pointer at the correct start position
	rc = fseek(file, startByte, SEEK_SET);
	assert(rc == 0);

	// Use fread to read however many bytes the segment is, at sizeof(char) (a character) per element
	rc = fread(readArray, sizeof(char), readBytes/sizeof(char), file);
	assert(rc == readBytes);

	fclose(file);

	char *word; //= malloc(STRMAX*sizeof(char));
	
	word = strtok_r(readArray, delimiters, &readArray);

	while(word)
	{
		node_t *newNode = malloc(sizeof(node_t));
		strncpy(newNode->word, word, STRMAX);
		newNode->next = NULL;
		newNode->count = 1;

		pthread_mutex_lock(lock);
		// Check if the buffer is too full
		if(*brSize >= bufferMaxSize)
		{
			// Buffer is too full, so we have to wait
			rc = pthread_cond_wait(cond, lock);
			assert(rc == 0);
		}
		// CRITICAL SECTION START****
		// Add word to buffer_read

		push(&newNode, buffer_read, brSize);
	
		// CRITICAL SECTION END******
		pthread_mutex_unlock(lock);

		// Signal that a node was added to the buffer
		pthread_cond_signal(cond);

		word = NULL;

		word = strtok_r(readArray, delimiters, &readArray);
	}

	//readArray-=readBytes;
	// Notify that mapper is done
	pthread_mutex_lock(lock);
	*mapperDone = 1;
	pthread_mutex_unlock(lock);
	pthread_cond_signal(cond);
	// Free stuff
	free(pointerToFree);
	//free(word);

	return NULL;
}

// 
void *map_adder(void *arg)
{	
	adder_arg_t* adder = (adder_arg_t*)arg;

	pthread_mutex_t *lock = adder->lock;
	pthread_cond_t *cond = adder->cond;
	int replica = adder->replica;
	int *mapperDone = adder->mapperDone;

	node_t **buffer_read = &buffer_reader_array[replica-1];
	node_t **buffer_write = &buffer_adder_array[replica-1];
	int *brSize = br_size_array[replica-1];
	int *bwSize = bw_size_array[replica-1];

	node_t *nothing = NULL;
	// While mapper is not done, or while buffer_read has items
	while(!(*mapperDone) || *brSize>0)
	{
		//pthread_mutex_unlock(lock);
		int rc = 0;
		pthread_mutex_lock(lock);
		if(*brSize<=0)
		{
			rc = pthread_cond_wait(cond, lock);
			assert(rc == 0);
		}
		// CRITICAL SECTION START****
		node_t* tempNode = popHead(buffer_read, brSize);

		pthread_mutex_unlock(lock);
		
		if(!tempNode)
		{
			// popped the head of the list, and somehow got nothing
			continue;
		}

		node_t *match = findMatch(buffer_write, tempNode->word, &nothing, *bwSize);

		// If a match is found 
		if(match)
		{
			++(match->count);
			free(tempNode);
			tempNode = NULL;
		}
		// buffer_write is either empty, or a match was not found
		else
		{
			push(&tempNode, buffer_write, bwSize);
		}

		// Alert other threads to the fact that there's now an open space in buffer_read
		pthread_cond_signal(cond);

		// Lock to check the mapperDone value along with brSize
		//pthread_mutex_lock(lock);
	}
	// When finished
	// Increment thread return count
	++(*threads_returned);
	// Notify mapper that it can exit?
	// Free any memory allocated here
	free(lock);
	free(cond);
	free(mapperDone);
	free(adder);

	return NULL;
}


// Adds a node to the head of the list
int push(node_t **node, node_t **head, int *size)
{
	if(!(node))
	{
		return -1;
	}
	
	(*node)->next = *head;
	*head = *node;
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
