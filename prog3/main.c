#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "cmpsc473mr.h"

int main()
{
	int replicas = 10;
	int bufferSize = 100;
	int *threads_returned = malloc(sizeof(int));
	*threads_returned = 0;

	node_t *finalList = NULL;
	
	node_t** buffer_reader_array;
	buffer_reader_array = malloc(sizeof(node_t*)*replicas);

	node_t **buffer_adder_array;
	buffer_adder_array = malloc(sizeof(node_t*)*replicas);

	int **bw_size_array = malloc(sizeof(int*)*replicas);

	for(int i=1; i<=replicas; i++)
	{

		buffer_adder_array[i-1] = NULL;
		buffer_reader_array[i-1] = NULL;
		bw_size_array[i-1] = malloc(sizeof(int));
		*(bw_size_array[i-1]) = 0;
		mapper(bufferSize, replicas, "input1.txt", threads_returned, i, &buffer_reader_array[i-1], &buffer_adder_array[i-1], bw_size_array[i-1]);
		printf("mapper: %d called\n", i);
	}

	printf("For loop finished.\n");
	while(*threads_returned<replicas);

	printf("Reducer stage started.\n");

	reducer(buffer_adder_array, &finalList, replicas, bw_size_array);

	printf("threads returnd:%d\n", *threads_returned);
	free(threads_returned);

	for(int i=0; i<replicas; i++)
	{
		node_t* current = buffer_adder_array[i];
		node_t* prev = NULL;
		while(current)
		{
			prev = current;
			current = current->next;

			prev->next = NULL;
			free(prev);
			prev = NULL;
		}
	}

	for(int i=0; i<replicas; i++)
	{
		node_t* current = buffer_reader_array[i];
		node_t* prev = NULL;
		while(current)
		{
			prev = current;
			current = current->next;

			prev->next = NULL;
			free(prev);
			prev = NULL;
		}
	}

	for(int i =0; i<replicas; i++)
	{
		free(bw_size_array[i]);
	}

	free(bw_size_array);
	free(buffer_adder_array);
	free(buffer_reader_array);
	
	for(int i = 0; i<replicas; i++)
	{
		buffer_adder_array[i] = NULL;
	}

	node_t *current = finalList;
	node_t *previous = NULL;

	while(current)
	{
		printf("word: %s count: %d \n", current->word, current->count);
		previous = current;
		current = current->next;
		free(previous);
		previous = NULL;
	}

	return 0;
}
