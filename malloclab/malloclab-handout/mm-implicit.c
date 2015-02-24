/*
 * mm.c
 * chihangw -- Chih-Ang Wang
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a full description of your solution.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "contracts.h"

#include "mm.h"
#include "memlib.h"

#define NEXT_FIT

// few Global variables and macros
static char *heap_listp = 0;  /* Pointer to first block */ 
#ifdef NEXT_FIT
    static char *rover;       /* Next fit rover */
#endif
/* Basic constants and macros */
#define WSIZE       4       /* Word and header/footer size (bytes) */ 
#define DSIZE       8       /* Doubleword size (bytes) */
#define CHUNKSIZE  (1<<15)  /* Extend heap by this amount (bytes) */

// Create aliases for driver tests
// DO NOT CHANGE THE FOLLOWING!
#ifdef DRIVER
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif

/*
 *  Logging Functions
 *  -----------------
 *  - dbg_printf acts like printf, but will not be run in a release build.
 *  - checkheap acts like mm_checkheap, but prints the line it failed on and
 *    exits if it fails.
 */

#ifndef NDEBUG
#define dbg_printf(...) printf(__VA_ARGS__)
#define checkheap(verbose) do {if (mm_checkheap(verbose)) {  \
                             printf("Checkheap failed on line %d\n", __LINE__);\
                             exit(-1);  \
                        }}while(0)
#else
#define dbg_printf(...)
#define checkheap(...)
#endif

/*
 *  Helper functions
 *  ----------------
 */

// Align p to a multiple of w bytes
static inline void* align(const void const* p, unsigned char w) {
    return (void*)(((uintptr_t)(p) + (w-1)) & ~(w-1));
}

// Check if the given pointer is 8-byte aligned
static inline int aligned(const void const* p) {
    return align(p, 8) == p;
}

// Return whether the pointer is in the heap.
static int in_heap(const void* p) {
    if (p > mem_heap_hi() || p < mem_heap_lo())
        printf("heap_hi: %p heap_lo: %p p: %p \n", mem_heap_hi(), mem_heap_lo(), p);
    return p <= mem_heap_hi() && p >= mem_heap_lo();
}


/*
 *  Block Functions
 *  ---------------
 *  TODO: Add your comment describing block functions here.
 *  The functions below act similar to the macros in the book, but calculate
 *  size in multiples of 4 bytes.
 */

// Return the size of the given block in multiples of the word size
static inline unsigned int block_size(const uint32_t* block) {
    REQUIRES(block != NULL);
    REQUIRES(in_heap(block));

    return (block[0] & 0x3FFFFFFF);
}

// Return true if the block is free, false otherwise
static inline int block_free(const uint32_t* block) {
    REQUIRES(block != NULL);
    REQUIRES(in_heap(block));

    return !(block[0] & 0x40000000);
}

// Mark the given block as free(1)/alloced(0) by marking the header and footer.
static inline void block_mark(uint32_t* block, int free) {
    REQUIRES(block != NULL);
    REQUIRES(in_heap(block));

    unsigned int next = block_size(block) + 1;
    block[0] = free ? block[0] & (int) 0xBFFFFFFF : block[0] | 0x40000000;
    block[next] = block[0];
}

// Return a pointer to the memory malloc should return
static inline uint32_t* block_mem(uint32_t* const block) {
    REQUIRES(block != NULL);
    REQUIRES(in_heap(block));
    REQUIRES(aligned(block + 1));

    return block + 1;
}

// Return the header to the previous block ---------------> Backward Traversal
static inline uint32_t* block_prev(uint32_t* const block) {
    REQUIRES(block != NULL);
    REQUIRES(in_heap(block));

    return block - block_size(block-1) - 2;
}

// Return the header to the next block     ---------------> Forward Traversal
static inline uint32_t* block_next(uint32_t* const block) {
    REQUIRES(block != NULL);
    REQUIRES(in_heap(block));

    return block + block_size(block) + 2;
}

/*
 *  Self defined Function by Chih-Ang
 *  ---------------------
 *  The following functions help decouple the dependency in my code.
 */

 // Pack the size and free/allocated bit within a block
static inline uint32_t pack(uint32_t size, int free) {
    return free ? (0xBFFFFFFF & size) : (0x40000000 | size);
}

static inline void put(void* block, uint32_t val) {
    *((uint32_t *)block) = val;
    return;
}

static inline size_t max(size_t a, size_t b) {
    return (a>b) ? a : b;
}

static void *coalesce(void *bp) {
    //printf("in coalece\n");
    uint32_t *block = (uint32_t *)bp - 1;
    int prev_free = block_free(block_prev(block));
    int next_free = block_free(block_next(block));
    int size_bp   = block_size(block); // in WSIZE

    if (!prev_free && !next_free) {                 /* Case 1 */
        return bp;
    } 
    else if (!prev_free && next_free) {             /* Case 2 */
        /* get size after coalescing */
        size_bp += block_size(block_next(block)) + 2;
        /* update the size in header and footer */
        put(block, pack(size_bp, 1));
        block_mark(block, 1);
        bp = (void *)block_mem(block);
    }
    else if (prev_free && !next_free) {             /* Case 3 */
        /* get size after coalescing */
        size_bp += block_size(block_prev(block)) + 2;
        /* update the size in header and footer */       
        put(block_prev(block), pack(size_bp, 1));
        block_mark(block_prev(block), 1);
        bp = (void *)block_mem(block_prev(block));
    }
    else {                                          /* Case 4 */
        /* get size after coalescing */
        size_bp += block_size(block_prev(block)) \
                +  block_size(block_next(block)) + 4;
        /* update the size in header and footer */
        put(block_prev(block), pack(size_bp, 1));
        block_mark(block_prev(block), 1);
        bp = (void *)block_mem(block_prev(block));
    }

#ifdef NEXT_FIT
    if ((rover > (char *)bp) && (rover < (char *)block_mem(block_next(block))))
        rover = (char *)bp;
#endif

    return bp;
}

static void *extend_heap(size_t words) {
    char *bp;
    size_t size;
    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    /* Initialize free block header/footer and the epilogue header */
    put(bp - WSIZE, pack(size/WSIZE-2, 1));                /* Free block header */    
    put(bp + size - WSIZE*2, pack(size/WSIZE-2, 1));       /* Free block footer */ 
    put(bp + size - WSIZE, pack(0, 0));                    /* New epilogue header */

    /* Coalesce if the previoue block (Case 3) is free */
    return coalesce(bp);
}

static void *find_fit(size_t asize) {

#ifdef NEXT_FIT
    /* Next fit search */
    uint32_t *block = (uint32_t *)rover - 1,
             *oldblock = (uint32_t *)rover - 1;  

    /* Search from rover to end of the list */
    for(; block_size(block) > 0; block = block_next(block)) {
        if (block_free(block) && block_size(block)*WSIZE >= asize)
            return (void *)block_mem(block);
    }

    /* Search from start of list to oldrover */
    block = (uint32_t *)heap_listp - 1;
    for(; block < oldblock; block = block_next(block)) {
        if (block_free(block) && block_size(block)*WSIZE >= asize)
            return (void *)block_mem(block);
    }

    /* No fit found */
    return NULL;
#else
    uint32_t *block = (uint32_t *)heap_listp - 1;

    for(; block_size(block) > 0; block = block_next(block)) {
        if(block_free(block) && block_size(block)*WSIZE >= asize)
            return  (void *)block_mem(block);
    }
    return NULL; // no fit
#endif
}

static void place(void *bp, size_t asize) {
    uint32_t *block = (uint32_t *)bp - 1;
    size_t csize = block_size(block) * WSIZE; // convert into bytes

    /* should insert back into the list */
    if((csize - asize) >= (2*DSIZE)) {
        // allocate teh asize block to the user
        put(block, pack(asize/WSIZE, 0));
        block_mark(block, 0);
        block = block_next(block);
        // free the remaining blocks
        // should deduct 2*WSIZE for header and footer
        put(block, pack(csize/WSIZE - asize/WSIZE - 2, 1));
        block_mark(block, 1);
    }
    /* just give all out */
    else {
        block_mark(block, 0);
    }
}


/*
 *  Malloc Implementation
 *  ---------------------
 *  The following functions deal with the user-facing malloc implementation.
 */

/*
 * Initialize: return -1 on error, 0 on success.
 */
int mm_init(void) {
    /* Create the initial empty heap */
    if ((heap_listp = mem_sbrk(6*WSIZE)) == (void *)-1)
       return -1;    
    put(heap_listp, 0);                                 /* Alignment padding */
    put(heap_listp + (1*WSIZE), pack(DSIZE/WSIZE, 0));      /* Prologue header */
    put(heap_listp + (2*WSIZE), pack(DSIZE/WSIZE, 0));      /* Prologue header */
    put(heap_listp + (3*WSIZE), pack(DSIZE/WSIZE, 0));      /* Prologue footer */ 
    put(heap_listp + (4*WSIZE), pack(DSIZE/WSIZE, 0));      /* Prologue footer */ 
    put(heap_listp + (5*WSIZE), pack(0, 0));            /* Epilogue header */
    heap_listp += 2*WSIZE;

#ifdef NEXT_FIT
    rover = heap_listp;
#endif

    if(extend_heap(CHUNKSIZE/WSIZE) == NULL) {
        return -1;
    }
    return 0;
}

/*
 * malloc
 */
void *malloc (size_t size) {
    size_t asize;      /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp; 
    
    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)                                         
       asize = 2*DSIZE;                                        
    else
       asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE); 

    /* Search for the free list */
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        checkheap(1);  // Let's make sure the heap is ok!
        if (!aligned(bp))
            printf("Not aligned (Found)\n");
        return bp;
    }
    /* No fit found. Get more memory and place the block */
    extendsize = max(asize, CHUNKSIZE);
    if((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    if (!aligned(bp))
        printf("Not aligned (Not Found)\n");
    place(bp, asize);
    checkheap(1);  // Let's make sure the heap is ok!
    return bp;
}

/*
 * free
 */
void free (void *ptr) {
    if (ptr == NULL) {
        return;
    }
    uint32_t *block = (uint32_t *)ptr - 1;

    if (heap_listp == NULL)
        mm_init();

    if (block_size(block) != block_size(block_next(block)-1))
        return;

    if (block_free(block))
        return;

    block_mark(block, 1);
    coalesce(ptr);
}

/*
 * realloc - you may want to look at mm-naive.c
 */
void *realloc(void *oldptr, size_t size) {
    size_t oldsize;
    void *newptr;
    uint32_t *block = (uint32_t *)oldptr - 1;

    /* If size == 0 then this is just free, and we return NULL. */
    if(size == 0) {
       free(oldptr);
       return 0;
    }

    /* If oldptr is NULL, then this is just malloc. */
    if(oldptr == NULL) {
       return malloc(size);
    }

    newptr = malloc(size);

    /* If realloc() fails, the original block is left untouched  */
    if(!newptr) {
       return 0;
    }

    /* Copy the old data. */
    oldsize = block_size(block)*WSIZE;
    if(size < oldsize) 
        oldsize = size;
    memcpy(newptr, oldptr, oldsize);

    /* Free the old block. */
    free(oldptr);

    return newptr;
}

/*
 * calloc - you may want to look at mm-naive.c
 */
void *calloc (size_t nmemb, size_t size) {
  size_t bytes = nmemb * size;
  void *newptr;

  newptr = malloc(bytes);
  memset(newptr, 0, bytes);

  return newptr;
}
/*
static void checkblock(void *bp) {
    uint32_t *block = (uint32_t *)bp - 1;
    if (!in_heap(block))
        printf("Error: %p is not in heap\n", (void *)block);
    if (aligned(bp))
        printf("Error: %p is not doubleword aligned\n", bp);
    if (block_size(block) != block_size(block+block_size(block)+1))
        printf("Error: header does not match footer\n");
}
*/

// Returns 0 if no errors were found, otherwise returns the error
int mm_checkheap(int verbose) {
/*
    char *bp;
    uint32_t *block = (uint32_t *)heap_listp - 1;    

    if (verbose)
        printf("Heap (%p):\n", heap_listp);

    if (block_size(block) != DSIZE/WSIZE || block_free(block)) {
        printf("Bad prologue header\n");
    }
    checkblock(heap_listp);
   
    for ( ; block_size(block) > 0; block = block_next(block)) {
        bp = (char *)block_mem(block);
        checkblock(bp);
    } 

    if (block_size(block) != 0 || block_free(block))
        printf("Bad epilogue header\n");

    return 0;
*/
    verbose = verbose;
    uint32_t *block = (uint32_t *)heap_listp - 1;    
    for ( ; block_size(block) > 0; block = block_next(block)) {
        if (!in_heap(block))
            printf("Error: %p not in heap\n", (void *)block);
    } 

    return 0;
}
