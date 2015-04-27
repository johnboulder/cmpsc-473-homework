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

int mapper(char *filename, int rep);

void reducer();

// 
void *map_reader(void *arg);

// 
void *map_adder(void *arg);
