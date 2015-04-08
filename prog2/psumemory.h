#pragma once
#include <stdlib.h>
#include "node.h"
#include <assert.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>

typedef struct __header_t
{
	int size;
	int magic;
} header_t;

void popNode(node_t *node);

void *psumalloc(int size);

int psufree(void* ptr);

int psumeminit(int algo, int sizeOfRegion);

void *bestFit(int size);

void *worstFit(int size);

void printFreeList();

void printHeaderSize(void *ptr);
