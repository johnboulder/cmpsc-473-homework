/**
 * NOTES:
 * What we're changing is just the way that we interact with the ready_list depending
 * on which scheduling algorithm we're implementing.
 */

#include "threads.h"
extern int scheduling_type;
extern thread_queue_t *ready_list;

void thread_enqueue(thread_t *t, thread_queue_t *q)
{
	// Create a new thread node to be added to the queue
	thread_node_t *newNode = (thread_node_t*) malloc(sizeof(thread_node_t));
	// Assign the new thread node's thread member to t
	newNode->thread = t;
	newNode->next = NULL;

	// If the queue is empty, our head and tail are the same
	if(q->size == 0)
	{
		// Set the tail to the newNode
		q->tail = newNode;
		// Set the head to the newNode
		q->head = newNode;
		// Set the head's next to the tail
		//q->head->next = q->tail;
	}
	else
	{
		// Connect our new thread node with the last node in the queue
		q->tail->next = newNode;
		// Assign the queue's tail to the new node
		q->tail = newNode;
	}


	++(q->size);
}

thread_t *thread_dequeue(thread_queue_t *q)
{
	thread_node_t *threadNode = q->head;

	if(threadNode)
	{
		if(threadNode->next)
		{
			q->head = q->head->next;
		}
		else
		{
			q->head = NULL;
		}
	}
	else
	{
		return NULL;
	}

	thread_t *t = threadNode->thread;
	free(threadNode);
	threadNode = NULL;
	--(q->size);

	return t; // You need to return real thread pointer
}

thread_t* scheduler()
{
	switch(scheduling_type)
	{
		// Round Robin
		case 0:
			{
				/* Write your code here*/
				thread_t *t;
				t = thread_dequeue(ready_list);
				//thread_enqueue(t, ready_list);
			
				// Return pointer of the next thread
				return t; 			
			}

		// Lottery
		case 1:          
			{
				/*Write your code here*/
				thread_node_t *current = ready_list->head;
				int count = 0;
				int winner = 0;
				int totalTickets = 0;
				thread_t *t;
				while(current)
				{
					totalTickets += current->thread->priority;
					current = current->next;
				}

				winner = random() % totalTickets;
				
				current = ready_list->head;
				thread_node_t *prev = NULL;
				while(current)
				{
					count += current->thread->priority;
					if(count>winner)
					{
						// Found the winner
						break;
					}
					prev = current;
					current = current->next;
				}

				if(prev && current)
				{
					prev->next = current->next;

					if(!prev->next)
					{
						ready_list->tail = prev;
					}

					--(ready_list->size);
					t = current->thread;
					free(current);
					current = NULL;
				}
				else if(!current)
				{
					// current is null, which should only happen when the list is empty
					return NULL;
				}
				else
				{
					t = thread_dequeue(ready_list);
				}
				// Enqueue the thread again to ensure that the it doesn't win every time over 
				// threads with the same priority
				thread_enqueue(t, ready_list);
				return t;
			}

		//First come first served
		case 2:          
			return ready_list->head->thread;

		default:
			return NULL;
	}
}
