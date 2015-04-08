#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "psumemory.h"

#define NUM 100

typedef struct _test{
    int a;
    char one;
    char two;
} test;

void write_test(test* ptr){
    if (ptr == NULL){
        return;
    }
    ptr->a = 1;
    ptr->one = 'A';
    ptr->two = 'B';
}

void read_test(test* ptr, FILE* f){
    if (ptr == NULL){
        return;
    }
    fprintf(f, "This struct contains: %d, %c and %c. \n", ptr->a, ptr->one, ptr->two);
}


int main(){
    FILE* f = fopen ("test_output7.txt", "w");
    int size;
    int sizeOfRegion = 1 << 20;// 1MB
    int a = psumeminit(0, sizeOfRegion);
    if (a == -1){
        fprintf(f, "Initialization failed!\n");
    }
    for (int i = 0; i < 10; i++){
        void* pointer_array[NUM];
        for (int i = 0; i < NUM; ++i){
            pointer_array[i] = NULL;
        }
        for (int j = 0; j < NUM; ++j)
        {
            size = rand()%248 + 8;
            test* a = (test*)psumalloc(size);
            write_test(a);
            read_test(a, f);
            if (a == NULL){
                fprintf(f, "No.%d : No extra space for allocation in memory!\n", j);
            }
            else{
                pointer_array[j] = a;
                fprintf(f, "NO.%d chunk has been allocated. the size is %d bytes \n", j, size);
            }
        }
        
        for (int j = 0; j < NUM; ++j)
        {
            int a = psufree(pointer_array[j]);
            if (a == 0){
                fprintf(f, "No.%d chunk has been freed. \n", j);
            }
            else{
                fprintf(f, "Can not free No.%d chunk. \n", j);
            }
        }
    }
    fclose(f);
    return 0;
}
