#include "psumemory.h"
#include <stdio.h>

typedef struct __test_t
{
	long double balls;
	struct __test_t* next;
}test_t;

int main()
{
	printf("Main Executed \n");
	psumeminit(1, 4097); 
	int *banana = psumalloc(sizeof(int));
	*banana = 5;
	printf("banana: %d\n", *banana);
	
	int *banana2 = psumalloc(sizeof(int));
	*banana2 = 8;
	printf("banana2: %d\n", *banana2);
	printf("banana2hdr size: %d\n", (((header_t*)banana2)-1)->size);
	printf("banana2hdr magic: %d\n", (((header_t*)banana2)-1)->magic);

	test_t *bigFuck = psumalloc(sizeof(test_t));
	test_t *bigFuck1 = psumalloc(sizeof(test_t));
	test_t *bigFuck2 = psumalloc(sizeof(test_t));

	bigFuck->next = bigFuck1;
	bigFuck1->next = bigFuck2;

	bigFuck->balls = 0.000000000;
	bigFuck1->balls = 1234123443.123423;

	test_t *current = bigFuck;
	while(current)
	{
		printf("balls: %Lf\n", current->balls);
		current = current->next;
	}
	psufree(banana);
	printf("Banana freed\n");
	banana = NULL;
	psufree(banana2);
	banana2 = NULL;
	printf("Banana2 freed\n");

	printFreeList();
	//printHeaderSize(bigFuck);
	int *banana3 = psumalloc(sizeof(int));
	//printHeaderSize(bigFuck); 
	//printf("magic: %d\n",(((header_t*)bigFuck)-1)->magic);


	*banana3 = 2;

	// Error Occurs here after the data for banana3 is allocated
	//printHeaderSize(bigFuck);
	printFreeList();
	
	//printf("size banana3: %d\n",(((header_t*)banana3)-1)->size);
	
	//printf("magic: %d\n",(((header_t*)bigFuck)-1)->magic);
	psufree(banana3);
	//printf("magic: %d\n",(((header_t*)bigFuck)-1)->magic);
	printf("Banana3 freed\n");
	printFreeList();
	// Magic Number invalid
	psufree(bigFuck);
	printf("bigFuck freed\n");
	printFreeList();
	psufree(bigFuck1);
	printf("bigFuck1 freed\n");
	printFreeList();
	psufree(bigFuck2);
	printf("bigFuck2 freed\n");
	printFreeList();

	test_t *bigFuck3 = psumalloc(sizeof(test_t));
	psufree(bigFuck3);
	printf("bigFuck3 freed\n");
	printFreeList();

	return 0;
}
