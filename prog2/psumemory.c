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
#include "psumemory.h"
#include "node.h"
#include <assert.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>

node_t *head;
int strat;
void *top;
void *bottom;
const int BEST_FIT = 0, WORST_FIT = 1, MAGIC = 1234567, MIN_SIZE = 16;

void printHeap();
void coalesce();

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
	printHeap();
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
	printf("PSUFREE############################################################################\n");
	//printFreeList();

	if(!ptr)
	{
		return -1;
	}

	//long long int t = (long long int)&top;
	//long long int b = (long long int)&bottom;
	//long long int p = (long long int)&ptr;

	//printf("Bottom - Top: %lld\n", b-t);
	//printf("Pointer: %lld\n", p);
	//printf("Bottom - Pointer: %lld\n", b-p);
	//printf("Pointer - Top: %lld\n", p-t);

	if(ptr<top || ptr>bottom)
	{
		//printf("Bad pointer given to free \n");
		//printFreeList();
		return -1;
	}

	printf("PTR in PSUFREE %p\n",ptr);
	header_t *headPtr = (((header_t*)ptr)-1);
	
	// Check that the magic number is valid
	if(headPtr->magic == MAGIC)
	{
		//printf("Magic Number Valid \n");
		int headerSize = headPtr->size;
		int totalSize = headerSize+sizeof(header_t);
		// Put a node where ptr used to reside
		node_t *released = (node_t*) headPtr;

		// Set released to a size prorated for node
		released->size = totalSize-sizeof(node_t);

		// Add released to the free list
		released->next = head;
		head = released;
		//coalesce();
		return 0;
	}
	else
	{
		//printf("Magic Number Invalid \n");
		return -1;
	}
}

void coalesce()
{
	node_t *current = head;
	int flag = 0;

	while(current)
	{
		node_t *top = current;
		node_t *bot = (((void*)current)+current->size+sizeof(node_t));
		
		node_t *currentC = head;

		while(currentC)
		{
			node_t *topIn = currentC;
			node_t *botIn = (((void*)currentC)+currentC->size+sizeof(node_t));

			//printf("top = %p\n", top);
			//printf("bot = %p\n", bot);
			//printf("topIn = %p\n", topIn);
			//printf("botIn = %p\n\n\n", botIn);

			if(topIn == bot)
			{
				//node_t *next = current->next;
				popNode(current);
				// Top is currentC
				currentC->size += current->size+sizeof(node_t);
				flag = 1;
				//current = next;
				break;
			}
			
			if(top == botIn)
			{
				node_t *next = currentC->next;
				popNode(currentC);
				current->size += currentC->size+sizeof(node_t);
				// recalculate bot
				bot = (((void*)current)+current->size+sizeof(node_t));
				currentC = next;
			}

			else
			{
				currentC = currentC->next;
			}
		}
		if(!flag)
		{
			
			current = current->next;
		}
		else
		{
			current = head;
			flag = 0;
		}
	}
}

void *bestFit(int size)
{
	node_t *current = head;
	node_t *best = NULL;
	printf("BESTFIT############################################################################\n");
	printFreeList();

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
	int currentTotalSize = current->size;
	// While current is not NULL
	while(current)
	{
		currentTotalSize = current->size + sizeof(node_t);
		if(best == NULL)
		{
			if(currentTotalSize >= totalSize )
			{
				best = current;
			}
		}
		else
		{
			if(currentTotalSize >= totalSize && (currentTotalSize < (best->size+sizeof(node_t))))
			{
				best = current;
			}
		}
		current = current->next;
	}

	if(best == NULL)
	{
		return NULL;
	}

	// Is there enough space to split the two?
	// TODO check that this is working correctly
	int remainderSizeOfBest = ((best->size+sizeof(node_t))-totalSize);
	if(remainderSizeOfBest>MIN_SIZE+8)
	{	// SPLIT THE TWO
		// Set the head to where best resides
		header_t *headPtr = (header_t*)best;

		// Assign a pointer to best's old position
		void *bestNew = best;

		// TODO CHECK
		// Use it to find best's new position
		bestNew = bestNew+totalSize;

		// Get best's size value
		int bsize = best->size;
		bsize+=sizeof(node_t);

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
	// Not enough space to split
	else
	{	
		popNode(best);
		header_t *headPtr = (header_t*)best;
		// TODO might not encompass some fringe cases
		headPtr->size = best->size + sizeof(node_t) - sizeof(header_t);
		headPtr->magic = MAGIC;

		return (headPtr+1);
	}
}

/**
 *
 * TODO set this up so that nodes are not removed, but instead replaced in the list with an accurate pointer
 * to the node's new position?
 *
 */
void popNode(node_t *node)
{
	if(head == node)
	{
		head = head->next;
		return;
	}

	// Node is not the head, so we have to find it
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
	printFreeList();
	// While current->next is not NULL
	while(current)
	{
		if(current->size > worst->size && current->size > totalSize)
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

	//else if(worst->size+sizeof(node_t) > totalSize)
	// There's enough space, split the two
	int remainderSizeOfBest = ((worst->size+sizeof(node_t))-totalSize);
	if(remainderSizeOfBest>MIN_SIZE+8)
	{	
		// TODO there's always space to split the two in the scenarios we have.


		// SPLIT THE TWO
		// Set the head to where worst resides
		header_t *headPtr = (header_t*)worst;
		// Assign a pointer to worst's old position
		// TODO are we ensuring that the worstNew pointer is actually incrementing like we think it is
		void *worstNew = worst;
		// Use it to find worst's new position
		worstNew+=totalSize;
		// Get worst's size value
		int wsize = worst->size+sizeof(node_t);
		// Remove worst from the list
		popNode(worst);
		// Subtract totalSize from worst's size
		wsize-=totalSize;
		// Move worst
		worst=worstNew;
		// Set worst's values
		worst->size = wsize;
		worst->next = head;
		// Add worst back into the list
		head = worst;
		// Set headPtr's values
		headPtr->size = totalSize-sizeof(header_t);
		printf("size allocated: %d\n", headPtr->size);
		headPtr->magic = MAGIC;

		return (headPtr+1);
	}
	// Not enough space to split
	else
	{	
		//TODO are we popping the head properly?
		popNode(worst);
		header_t *headPtr = (header_t*)worst;
		// TODO might not encompass some fringe cases
		headPtr->size = worst->size + sizeof(node_t) - sizeof(header_t);
		printf("size allocated: %d\n", headPtr->size);
		headPtr->magic = MAGIC;

		return (headPtr+1);
	}
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

	top = (void*) head;
	bottom = ((void*)head)+adjustedSize; 

	return 0;
}

void printFreeList()
{
	printf("----------------------------------\n");
	printf("Printing Free List:\n");
	node_t *current = head;
	while(current)
	{
		printf("%p -> size:%d next:%p \n",current,current->size, current->next);
		current = current->next;
	}
	printf("----------------------------------\n\n\n");
}

void printHeaderSize(void *ptr)
{
	header_t *headPtr = (header_t*) ptr-1;
	printf("Header size:%d\n", headPtr->size);
}

void printHeap()
{
	void *current = top;
	while(&current<&bottom)
	{
		if(((header_t*)current)->magic == MAGIC)
		{
			printf("magic:%d\n", ((header_t*)current)->magic);
		}
		current+=1;
	}
}
