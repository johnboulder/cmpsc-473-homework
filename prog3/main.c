#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "cmpsc473mr.h"

int main()
{
	int replicas = 5;
	int bufferSize = 5;
	int *threads_returned = malloc(sizeof(int));
	*threads_returned = 0;

	node_t** buffer_adder_array = malloc(sizeof(node_t*)*replicas);

	for(int i=1; i<=replicas; i++)
	{
		buffer_adder_array[i] = malloc(sizeof(node_t));
		buffer_adder_array[i] = mapper(bufferSize, replicas, "input1.txt", threads_returned, i);
		printf("mapper:%d called\n", i);
	}

	printf("For loop finished.\n");
	while(*threads_returned<replicas);

	reducer();

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
			free(prev->word);
			prev->word = NULL;
			free(prev);
			prev = NULL;
		}
	}

	free(buffer_adder_array);

	buffer_adder_array = NULL;

	return 0;
}
