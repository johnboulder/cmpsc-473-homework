#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "psumemory.h"

#define NUM 100

typedef struct _test{
    int a;
    int b;
    int c;
    char one;
    char two;
    char three;
} test;

void write_test(test* ptr){
    if (ptr == NULL){
        return;
    }
    ptr->a = 1;
    ptr->b = 2;
    ptr->c = 3;
    ptr->one = 'A';
    ptr->two = 'B';
    ptr->three = 'C';
}

void read_test(test* ptr, FILE* f){
    if (ptr == NULL){
        return;
    }
    fprintf(f, "This struct contains: %d, %d, %d, %c, %c and %c. \n", ptr->a, ptr->b, ptr->c, ptr->one, ptr->two, ptr->three);
}

int main(){
	FILE* f = fopen ("test_output3.txt", "w");
	int size;
	int sizeOfRegion = 1 << 20;// 1MB
	int a = psumeminit(0, sizeOfRegion);
	if (a == -1){
		fprintf(f, "Initialization failed!\n");
	}
	for (int i = 0; i < 10; i++){
		void* pointer_array[NUM];
        for (int i = 0; i < NUM; i++){
            pointer_array[i] = NULL;
        }
		for (int i = 0; i < NUM; ++i)
		{
			if (i % 2 == 0){
				size = 64;
			}
			else{
				size = 64 * 1024;
			}
			test* a = (test*)psumalloc(size);
            write_test(a);
            read_test(a, f);
			if (a == NULL){
				fprintf(f, "No extra space for allocation in memory!\n");
			}
			else{
				pointer_array[i] = a;
				fprintf(f, "NO.%d chunk has been allocated. the size is %d bytes\n", i, size);
			}
		}

		for (int i = 0; i < NUM; ++i)
		{
			int a = psufree(pointer_array[i]);
			if (a == 0){
				fprintf(f, "No.%d chunk has been freed. \n", i);
			}
			else{
				fprintf(f, "Can not free No.%d chunk. \n", i);
			}
		}
	}
	fclose(f);
	return 0;
}