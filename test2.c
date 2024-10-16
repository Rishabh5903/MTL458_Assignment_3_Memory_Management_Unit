#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <time.h>
#include "my_mmu.h"

// int main() {
//     for (int i = 0; i < 10000; ++i) {
//         int* p = (int*)my_malloc(sizeof(int) * 10);
//         if (p) {
//             for (int j = 0; j < 10; ++j) {
//                 p[j] = j;
//             }
//             my_free(p);
//         }
//     }
//     printf("fafa\n");
//     return 0;
// }

int main() {
    // Step 1: Allocate multiple memory blocks
    int* a = (int*) my_malloc(sizeof(int));  // Block 1
    int* b = (int*) my_malloc(sizeof(int));  // Block 2
    int* c = (int*) my_malloc(sizeof(int));  // Block 3
    int* d = (int*) my_malloc(sizeof(int));  // Block 4
    int* e = (int*) my_malloc(sizeof(int));  // Block 5
    int* f = (int*) my_malloc(sizeof(int));  // Block 6
    int* s = (int*) malloc(sizeof(int));  // Block 1
    int* t = (int*) malloc(sizeof(int));  // Block 2
    int* u = (int*) malloc(sizeof(int));  // Block 3
    int* v = (int*) malloc(sizeof(int));  // Block 4
    int* w = (int*) malloc(sizeof(int));  // Block 5
    int* y = (int*) malloc(sizeof(int));  // Block 6

    // Assign values to the allocated blocks
    *a = 10; *b = 20; *c = 30; *d = 40; *e = 50; *f = 60;

    // Print allocated block addresses and values
    printf("Addresses and values:\n");
    printf("a: %p, value: %d\n", (void*)a, *a);
    printf("b: %p, value: %d\n", (void*)b, *b);
    printf("c: %p, value: %d\n", (void*)c, *c);
    printf("d: %p, value: %d\n", (void*)d, *d);
    printf("e: %p, value: %d\n", (void*)e, *e);
    printf("f: %p, value: %d\n", (void*)f, *f);

// / Assign values to the allocated blocks
    *s = 10; *t = 20; *u = 30; *v= 40; *w = 50; *y = 60;

    // Print allocated block addresses and values
    printf("Addresses and values:\n");
    printf("s: %p, value: %d\n", (void*)s, *s);
    printf("t: %p, value: %d\n", (void*)t, *t);
    printf("u: %p, value: %d\n", (void*)u, *u);
    printf("v: %p, value: %d\n", (void*)v, *v);
    printf("w: %p, value: %d\n", (void*)w, *w);
    printf("y: %p, value: %d\n", (void*)y, *y);
    // Step 2: Free non-adjacent blocks
    printf("\nFreeing blocks b and e...\n");
    my_free(b);  // Free Block 2
    my_free(e);  // Free Block 5

    // Allocate a new block after freeing b and e, check reuse
    int* g = (int*) my_malloc(sizeof(int));  // This should reuse Block 2's memory (b)
    *g = 70;
    printf("Allocated g at address %p (reusing b's memory), value: %d\n", (void*)g, *g);

    // Step 3: Free adjacent blocks (a and c) to test coalescing
    printf("\nFreeing blocks a and c (adjacent)...\n");
    my_free(a);  // Free Block 1
    my_free(c);  // Free Block 3

    // Allocate a new larger block, which should coalesce a and c
    int* h = (int*) my_malloc(2 * sizeof(int));  // This should reuse the coalesced block (a + c)
    h[0] = 80;
    h[1] = 90;
    printf("Allocated h at address %p (reusing coalesced a and c), values: %d, %d\n", (void*)h, h[0], h[1]);
    
    // Print the locations of h[0] and h[1]
    printf("h[0] is at address %p, value: %d\n", (void*)&h[0], h[0]);
    printf("h[1] is at address %p, value: %d\n", (void*)&h[1], h[1]);

    // Step 4: Free adjacent blocks (d and f) to test further coalescing
    printf("\nFreeing blocks d and f (adjacent)...\n");
    my_free(d);  // Free Block 4
    my_free(f);  // Free Block 6

    // Allocate a new large block, which should reuse the coalesced blocks (d + f)
    int* i = (int*) my_malloc(2 * sizeof(int));  // This should reuse the coalesced block (d + f)
    i[0] = 100;
    i[1] = 110;
    printf("Allocated i at address %p (reusing coalesced d and f), values: %d, %d\n", (void*)i, i[0], i[1]);
    
    // Print the locations of i[0] and i[1]
    printf("i[0] is at address %p, value: %d\n", (void*)&i[0], i[0]);
    printf("i[1] is at address %p, value: %d\n", (void*)&i[1], i[1]);

    // Step 5: Free the remaining blocks to check if everything is handled correctly
    printf("\nFreeing remaining blocks (g, h, i)...\n");
    my_free(g);
    my_free(h);
    my_free(i);

    return 0;

}

// int main() {
//     // Allocate memory for 3 integers
//     int* a = (int*) my_malloc(sizeof(int));
//     int* b = (int*) my_malloc(sizeof(int));
//     int* c = (int*) my_malloc(sizeof(int));

//     *a = 100;
//     *b = 200;
//     *c = 300;

//     printf("Address of a: %p, b: %p, c: %p\n", (void*)a, (void*)b, (void*)c);

//     // Free the middle block (b)
//     my_free(b);
//     printf("Freed memory at address: %p\n", (void*)b);

//     // Allocate another block, which should reuse the free block (b's memory)
//     int* d = (int*) my_malloc(sizeof(int));
//     *d = 400;
//     printf("Address of d: %p (should reuse b's memory)\n", (void*)d);
//     printf("d = %d\n", *d);

//     // Free all remaining memory
//     my_free(a);
//     my_free(c);
//     my_free(d);

// }


// int main(){
//     printf("Size of META_SIZE: %zu bytes\n", META_SIZE);
// int* a = (int*) my_malloc(50 * sizeof(int));  // Block 1
// int* b = (int*) my_malloc(50 * sizeof(int));  // Block 2
// int* c = (int*) my_malloc(50 * sizeof(int));  // Block 3

// printf("Allocated blocks a, b, c:\n");
// printf("Meta block a: %p, user block a: %p\n", (void*)((struct headerstruct*)a - 1), (void*)a);
// printf("Meta block b: %p, user block b: %p\n", (void*)((struct headerstruct*)b - 1), (void*)b);
// printf("Meta block c: %p, user block c: %p\n", (void*)((struct headerstruct*)c - 1), (void*)c);

// // Free adjacent blocks a and b
// my_free(a);
// printf("freea");
// my_free(b);
// printf("freeb");
// printf("Freed blocks a and b.\n");

// // Allocate a new large block, it should reuse coalesced space of a + b
// int* d = (int*) my_malloc(90 * sizeof(int));  // Block should coalesce a + b
// printf("Allocated block d, should reuse a + b:\n");
// printf("Meta block d: %p, user block d: %p\n", (void*)((struct headerstruct*)d - 1), (void*)d);

// // Free remaining blocks
// my_free(c);
// printf("freec...");
// my_free(d);
// printf("freed..");
// printf("Freed blocks c and d.\n");
// }