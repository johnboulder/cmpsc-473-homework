#define NUM 100
#include <stdio.h>
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
    FILE* f = fopen ("test_output2.txt", "w");
    int size;
    int sizeOfRegion = 1 << 20;// 1MB
    int a = psumeminit(1, sizeOfRegion);
    if (a == -1){
        fprintf(f, "Initialization failed!\n");
    }
    
    void* pointer_array[16][NUM];
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < NUM; j++) {
            pointer_array[i][j] = NULL;
        }
    }
    
    for (int i = 0; i < 16; i++){
        for (int j = 0; j < NUM; ++j)
        {
            if (j % 2 != 0){
                size = 64;
            }
            else{
                size = 64 * 1024;
            }
            test* a = (test*)psumalloc(size);
            write_test(a);
            read_test(a, f);
            if (a == NULL){
                fprintf(f, "No.%d has no extra space for allocation in memory!\n", j);
            }
            else{
                pointer_array[i][j] = a;
                fprintf(f, "NO.%d chunk has been allocated, the size is %d bytes\n", j, size);
            }
        }
    }
    
    int half = 16;
    while (half != 0){
        half /= 2;
        int bound;
        if (half){
            bound = half * 2;
        }
        else{
            bound = 1;
        }
        for (int i = half; i < bound; i++){
            for (int j = 0; j < NUM; j++)
            {
                int a = psufree(pointer_array[i][j]);
                if (a == 0){
                    fprintf(f, "No.%d chunk has been freed. \n", j);
                }
                else{
                    fprintf(f, "Can not free No.%d chunk. \n", j);
                }
            }
        }
    }
    fclose(f);
    return 0;
}