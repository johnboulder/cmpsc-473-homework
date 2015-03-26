/**
 * Goal: Implement my own versions of malloc and free
 *
 * Hints: 
 * Use sbrk() and mmap()
 *
 * To avoid corruption in multithreaded applications, mutexes are used internally to protect the memory management data
 * structures employed by these functions.
 *
 * Set errno to ENOMEM, indicate what went wrong when something does go wrong
 *
 *
 */

#include "prog.h"
#include "node.h"
#include <assert.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>

node_t *head;
int strat;
const int BEST_FIT = 0, WORST_FIT = 1, MAGIC = 1234567, MIN_SIZE = 16;

void *psumalloc(int size)
{
	// 1. Find a chunk in our list that is big enough to service the size requested
	// 2. Split that chunk into to pieces, 1 that is size+sizeof(header_t), and one that is whatever remains
	// 3.
	//
	// Growing the heap
	// Look into useage of brk and sbrk for this. Those are what are used to accomplish growing the heap.
	void *ptr;
	if(strat == BEST_FIT)
	{
		ptr = bestFit(size);
	}
	else if(strat == WORST_FIT)
	{
		ptr = worstFit(size);
	}
	else
	{
		return NULL;
	}

	return ptr;
}

int psufree(void* ptr)
{
	// Allocators store a little bit of extra info in the header block which is kept in memory, usually just before the handed out chunk of memory.
	// Find the header block assuming that the ptr value is at the top of the heap
	//header_t *hptr = (void *)ptr - sizeof(header_t);
	// Check that the magic number equals the expected value as a sanity check
	//assert(hptr->magic == 1234567);
	// Through simple math find the size of the newly freed region
	// i.e. the size of the header + the size of the region.
	
	// 1. Find the header of ptr
	// 2. Take that virtual address and find the header block
	// 3. Use the size information contained in the header block to set the size info in a node and place that node in the same position the header was in
	// 4. Insert it at the head of the free list
	// 5. Check if we can coelesce


	header_t *headPtr = (((header_t*)ptr)-1);
	
	// Check that the magic number is valid
	if(headPtr->magic == MAGIC)
	{
		printf("Magic Number Valid \n");
		int headerSize = headPtr->size;
		int totalSize = headerSize+sizeof(header_t);
		// Put a node where ptr used to reside
		node_t *released = (node_t*) ptr;

		// Set released to a size prorated for node
		released->size = totalSize-sizeof(node_t);

		// Add released to the free list
		released->next = head;
		head = released;
		return 0;
	}
	else
	{
		printf("Magic Number Invalid \n");
		return -1;
	}
}

void *bestFit(int size)
{
	node_t *current = head->next;
	node_t *best = head;

	// TODO fix our program such that we don't need to do this.
	// We do this here because freeing memory requires that we have enough space to place a node_t
	// in place of the header_t in the memory slot and it's corrupting contiguous parts of memory. 
	// We can remedy this by ensuring that node_t and header_t are the same size, or by just using
	// the same struct in place of them both.
	if(size < MIN_SIZE)
	{
		size = MIN_SIZE;
	}
	int totalSize = size + sizeof(header_t);

	// While current->next is not NULL
	while(current)
	{
		if(current->size < best->size && current->size >= totalSize)
		{
			best = current;
		}
		current = current->next;
	}

	// Is the size of best smaller than totalSize? Case where best==head still
	if(best->size < totalSize)
	{
		return NULL;
	}

	// There's enough space, split the two
	if(best->size > totalSize)
	{	// SPLIT THE TWO
		// Set the head to where best resides
		header_t *headPtr = (header_t*)best;
		// Assign a pointer to best's old position
		void *bestNew = best;
		// Use it to find best's new position
		bestNew+=totalSize;
		// Get best's size value
		int bsize = best->size;
		// Remove best from the list
		popNode(best);
		// Subtract totalSize from best's size
		bsize-=totalSize;
		// Move best
		best=bestNew;
		// Set best's values
		best->size = bsize;
		best->next = head;
		// Add best back into the list
		head = best;

		// Set headPtr's values
		headPtr->size = totalSize-sizeof(header_t);
		headPtr->magic = MAGIC;

		return (headPtr+1);
	}
	else if(best->size == totalSize)
	{
		best->size-=totalSize;
		node_t *endBest = best+1;
		header_t *headPtr = (header_t*)endBest;
		headPtr->size = totalSize-sizeof(header_t);
		headPtr->magic = MAGIC;

		return (headPtr+1);
	}

	return NULL;
}

/**
 *
 * TODO set this up so that nodes are not removed, but instead replaced in the list with an accurate pointer
 * to the node's new position.
 *
 */
void popNode(node_t *node)
{
	// Find node
	node_t *current = head->next;
	node_t *prev = head;
	while(current)
	{
		if(current == node)
		{
			// Node found. Remove it.
			prev->next = current->next;
			current->next = NULL;
			return;
		}
		prev = current;
		current = current->next;
	}

	// 1 element in list, so set head to NULL
	if(prev == head && current == NULL)
	{
		head = NULL;
	}
}

void *worstFit(int size)
{
	node_t *current = head->next;
	node_t *worst = head;

	// TODO fix our program such that we don't need to do this.
	// We do this here because freeing memory requires that we have enough space to place a node_t
	// in place of the header_t in the memory slot and it's corrupting contiguous parts of memory. 
	// We can remedy this by ensuring that node_t and header_t are the same size, or by just using
	// the same struct in place of them both.
	if(size < MIN_SIZE)
	{
		size = MIN_SIZE;
	}
	int totalSize = size + sizeof(header_t);

	// While current->next is not NULL
	while(current)
	{
		if(current->size < worst->size && current->size > totalSize)
		{
			worst = current;
		}
		current = current->next;
	}

	// Is the size of worst smaller than totalSize? Case where worst==head still
	if(worst->size < totalSize)
	{
		return NULL;
	}

	// There's enough space, split the two
	if(worst->size > totalSize)
	{	// SPLIT THE TWO
		// Set the head to where worst resides
		header_t *headPtr = (header_t*)worst;
		// Assign a pointer to worst's old position
		void *worstNew = worst;
		// Use it to find worst's new position
		worstNew+=totalSize;
		// Get worst's size value
		int bsize = worst->size;
		// Remove worst from the list
		popNode(worst);
		// Subtract totalSize from worst's size
		bsize-=totalSize;
		// Move worst
		worst=worstNew;
		// Set worst's values
		worst->size = bsize;
		worst->next = head;
		// Add worst back into the list
		head = worst;

		// Set headPtr's values
		headPtr->size = totalSize-sizeof(header_t);
		headPtr->magic = MAGIC;

		return (headPtr+1);
	}
	else if(worst->size == totalSize)
	{
		worst->size-=totalSize;
		node_t *endWorst = worst+1;
		header_t *headPtr = (header_t*)endWorst;
		headPtr->size = totalSize-sizeof(header_t);
		headPtr->magic = MAGIC;

		return (headPtr+1);
	}

	return NULL;
}

/**
 * PARAMS:
 * algo - memory allocation policy to use. 0 for best-fit, 1 for worst-fit
 * sizeOfRegion - number of bytes you request from the OS using mmap
 * RETURN:
 * 0 if successful
 * -1 if otherwise
 */
int psumeminit(int algo, int sizeOfRegion)
{
	// 1. Set a strategy flag for using either bestfit or worstfit
	strat = algo;

	int adjustedSize = (sizeOfRegion<getpagesize())? getpagesize():(sizeOfRegion/getpagesize())*getpagesize();
	adjustedSize += (sizeOfRegion%getpagesize())? getpagesize():0;

	head = mmap(NULL, adjustedSize, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);

	head->size = adjustedSize-sizeof(node_t);
	head->next = NULL;

	return 0;
}

void printFreeList()
{
	printf("Printing Free List:\n");
	node_t *current = head;
	while(current)
	{
		printf("size:%d\n", current->size);
		current = current->next;
	}
}
