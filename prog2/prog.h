#include <stdlib.h>
#include "node.h"

typedef struct __header_t
{
	int size;
	int magic;
} header_t;

void popNode(node_t *node);

void *psumalloc(int size);

void free(void* ptr);

int psumeminit(int algo, int sizeOfRegion);

void *bestFit(int size);

void *worstFit(int size);
