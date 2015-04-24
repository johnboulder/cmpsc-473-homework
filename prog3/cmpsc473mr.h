// MAPPER STAGE
// PRODUCER: map-reader-i adds entries to buffer1 in the form (word, 1) for each word (buffer1 is bounded in size)
// CONSUMER: map-adder-i reads entries from buffer1 and adds them to buffer2 in the form of (word, count) (buffer2 is not bounded)
// Since buffer2 exists for each producer/consumer pair, the buffers produced will have to be summed. The reducer stage does this summing
// REDUCER STAGE
// Single threaded operation. Only runs once, and only after the final map-adder-n has finished running

typedef struct __node_t
{
	struct __node_t *next;
	char word[25];
	int count;
} node_t;

// Returns a pointer to the second buffer that was made in this function. i.e. buffer_write
int mapper(int bMax, int reps, char *filename, int *thread_return_count, int rep, node_t** buffer_read, node_t** buffer_write);

void reducer();

// 
void *map_reader(void *arg);

// 
void *map_adder(void *arg);
