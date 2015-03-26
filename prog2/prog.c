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

node_t *head;
int strat;
const int BEST_FIT = 0, WORST_FIT = 1;

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

void psufree(void* ptr)
{
	// Allocators store a little bit of extra info in the header block which is kept in memory, usually just before the handed out chunk of memory.
	// Find the header block assuming that the ptr value is at the top of the heap
	//header_t *hptr = (void *)ptr - sizeof(header_t);
	// Check that the magic number equals the expected value as a sanity check
	//assert(hptr->magic == 1234567);
	// Through simple math find the size of the newly freed region
	// i.e. the size of the header + the size of the region.
	
	// 1. Get the virtual address from ptr
	
	// 2. Take that virtual address and find the header block
	// 3. Use the size information contained in the header block to set the size info in a node and place that node in teh same position the header was in
	// 4. Insert it at the head of the free list
	// 5. Check if we can coelesce
}

void *bestFit(int size)
{
	node_t *current = head->next;
	node_t *best = head;
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
	{	
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
		headPtr->magic = 1234567;

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
	int remain = (sizeOfRegion%getpagesize());
	// TODO Is it ok that we add on the size of node_t for allocation?
	int adjustedSize = remain*getpagesize() + sizeOfRegion-remain + sizeof(node_t);

	if(sizeOfRegion == remain)
	{
		adjustedSize = sizeOfRegion + sizeof(node_t);
	}

	// TODO Add a check here for any signals thrown?
	head = mmap(NULL, adjustedSize, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);

	head->size = adjustedSize-sizeof(node_t);
	head->next = NULL;

	return 0;
}
