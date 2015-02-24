/*
 * mm.c
 * chihangw -- Chih-Ang Wang
 *
 *
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

// Global variables and macros
static uint32_t *heap_listp = NULL;  /* Pointer to Prologue */ 
static uint32_t *ptr1 = NULL;        /* Pointer to first block in Class 1 */ 
static uint32_t *ptr2 = NULL;        /* Pointer to first block in Class 2 */
static uint32_t *ptr3 = NULL;        /* Pointer to first block in Class 3 */
static uint32_t *ptr4 = NULL;        /* Pointer to first block in Class 4 */
static uint32_t *ptr5 = NULL;        /* Pointer to first block in Class 5 */
static uint32_t *ptr6 = NULL;        /* Pointer to first block in Class 6 */
static uint32_t *ptr7 = NULL;        /* Pointer to first block in Class 7 */
static uint32_t *ptr8 = NULL;        /* Pointer to first block in Class 8 */
static uint32_t *ptr9 = NULL;        /* Pointer to first block in Class 9 */
static uint32_t *ptr10 = NULL;       /* Pointer to first block in Class 10 */
static uint32_t *ptr11 = NULL;       /* Pointer to first block in Class 11 */
static uint32_t *ptr12 = NULL;       /* Pointer to first block in Class 12 */
static uint32_t *ptr13 = NULL;       /* Pointer to first block in Class 13 */

/* Basic constants and macros */
#define WSIZE       4       /* Word and header/footer size (bytes) */ 
#define DSIZE       8       /* Doubleword size (bytes) */
#define CHUNKSIZE  (1<<8)  /* Extend heap by this amount (bytes) */
#define MINI_BLOCK 24       /* hdr(4) prev_ptr(8) next_ptr(8) ftr(4) */

/* bit tricks to flag allocated/free */
#define CURRENT_ALLOC     1
#define PREVIOUS_ALLOC    2
#define NEXT_ALLOC        4
#define N_SIZE_CLASS     13

/* size class. Multiples of MINI_BLK for small, Power of 2 for large request */
#define SEG_CLASS_1     24
#define SEG_CLASS_2     48
#define SEG_CLASS_3     72
#define SEG_CLASS_4     96
#define SEG_CLASS_5     120
#define SEG_CLASS_6     480
#define SEG_CLASS_7     960
#define SEG_CLASS_8     1920
#define SEG_CLASS_9     3840
#define SEG_CLASS_10    7680
#define SEG_CLASS_11    15360
#define SEG_CLASS_12    30720
#define SEG_CLASS_13    61440

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
static inline int in_heap(const void* p) {
    if (p > mem_heap_hi() || p < mem_heap_lo())
        printf("heap_hi: %p heap_lo: %p p: %p \n", mem_heap_hi(), mem_heap_lo(), p);
    return p <= mem_heap_hi() && p >= mem_heap_lo();
}


/*
 *  Self defined inline Functions by Chih-Ang
 *  ---------------------
 *  The following inline functions tell the compiler to expand this snippet.
 */

/* Pack the size (bytes) and free/allocated bit into a word */
static inline uint32_t PACK(uint32_t size, int alloc) {
    return (size | alloc);
}

/* Read / Write a word at address p */
static inline void PUT_WORD(void* p, uint32_t val) {
    *((uint32_t *)p) = val;
    return;
}

static inline uint32_t GET_WORD(void *p) {
    return *((uint32_t *)p);
}

/* Read the size (bytes) and allocated fields from address p */
static inline uint32_t GET_SIZE(void *p) {
    return (GET_WORD(p) & ~0x7);
}

static inline uint32_t GET_ALLOC(void *p) {
    return (GET_WORD(p) & 0x1);
}

static inline uint32_t GET_PRE_ALLOC(void *p) {
    return (GET_WORD(p) & 0x2);
}

static inline uint32_t GET_NXT_ALLOC(void *p) {
    return (GET_WORD(p) & 0x4);
}

/* Given block ptr bp, compute address of its header and footer */
static inline void *HDRP(void *bp) {
    return (void *)((char *)bp - WSIZE);
}

static inline void *FTRP(void *bp) {
    return (void *)((char *)bp + GET_SIZE(HDRP(bp)) - DSIZE);
}

/* Given block ptr bp, compute address of next and previous blocks */
/* We still need these functions for coalescing adjacent blocks !! */
static inline void *NEXT_BLKP(void *bp) {
    return (void *)((char *)bp + GET_SIZE((char *)bp - WSIZE));
}

static inline void *PREV_BLKP(void *bp) {
    return (void *)((char *)bp - GET_SIZE((char *)bp - DSIZE));
}

/* Given block ptr bp, return address to previous/next block */
static inline size_t PREDECESSOR(void *bp) {
    return *((size_t *)bp);
}

static inline size_t SUCCESSOR(void *bp) {
    return *((size_t *)bp + 1);
}

/* Return Max / Min of pair a, b */
static inline size_t MAX(size_t a, size_t b) {
    return (a>b) ? a : b;
}

static inline size_t MIN(size_t a, size_t b) {
    return (a>b) ? b : a;
}

/*
 *  End Self defined inline Functions by Chih-Ang
 */

/*
 *  Self defined helper Functions by Chih-Ang
 *  ---------------------
 *  The following functions help decouple dependency in my code.
 */

/* Return proper Ptr that the current bp should be inserted next to based on */
/* Address-Ordered Policy */
// static void *findLocation(uint32_t *classhead, void *current_bp) {
//     void *bp; 
//     for (bp = (void *)classhead; SUCCESSOR(bp) != NULL; bp = SUCCESSOR(bp)) {
//         if(bp < current_bp && current_bp < SUCCESSOR(bp))
//             return bp;
//     }
//     return current_bp;
// }

/* Classify the given size (bytes) and return ptr to head of the class */
static uint32_t **getClassHead(size_t size) {
    uint32_t **head;
    if(size <= SEG_CLASS_1)
        head = &ptr1;
    else if(size <= SEG_CLASS_2)
        head = &ptr2;
    else if(size <= SEG_CLASS_3)
        head = &ptr3;
    else if(size <= SEG_CLASS_4)
        head = &ptr4;
    else if(size <= SEG_CLASS_5)
        head = &ptr5;
    else if(size <= SEG_CLASS_6)
        head = &ptr6;
    else if(size <= SEG_CLASS_7)
        head = &ptr7;
    else if(size <= SEG_CLASS_8)
        head = &ptr8;
    else if(size <= SEG_CLASS_9)
        head = &ptr9;
    else if(size <= SEG_CLASS_10)
        head = &ptr10;
    else if(size <= SEG_CLASS_11)
        head = &ptr11;
    else if(size <= SEG_CLASS_12)
        head = &ptr12;
    else
        head = &ptr13;
    return head;
}

/* Insert the list pointed by bp into corresponding size class. The insertion */
/* policy is simply put the inserted block in the front of the list */
static void insertList(void *bp) {
    size_t size = GET_SIZE(HDRP(bp));

    /* Get address of the Class Head that *bp (new first block) belongs to */
    /* The return Ptr points to one of the ptr1 ~ ptr13 !! */
    uint32_t **class_head = getClassHead(size); 

    // uint32_t *insert_point = findLocation(*classhead, bp);

    /* Preserve the original address where the Class Head points */
    uint32_t *buffer = *class_head;
    /* Update the Class Head to the new first block address */
    *class_head = (uint32_t *)bp;

    /* The new first block's prev_ptr points to Class Head, which is NULL */
    *((size_t *)bp) = 0;
    /* The new first block's next_ptr points to the previous first block */
    *((size_t *)bp + 1) = (size_t)buffer; 
    /* if the previous first block is not NULL, we should update its prev_ptr */
    /* The previous first block's prev_ptr points to the current first block */
    if(buffer != NULL) {
        *((size_t *)buffer) = (size_t)bp;   
    }
}

/* Remove the list pointed by bp from the linked list chain */
static void removeList(void *bp) {

    uint32_t *successor_bp = (uint32_t *)SUCCESSOR(bp);
    uint32_t *predecessor_bp = (uint32_t *)PREDECESSOR(bp);

    if(!predecessor_bp && !successor_bp) {
        size_t size = GET_SIZE(HDRP(bp));
        uint32_t **class_head = getClassHead(size);
        *class_head = successor_bp;
    }
    else if(!predecessor_bp && successor_bp) {
        size_t size = GET_SIZE(HDRP(bp));
        uint32_t **class_head = getClassHead(size);
        *class_head = successor_bp;
        *((size_t *) successor_bp) = 0;
    }
    else if(predecessor_bp && !successor_bp) {
        *((size_t *)predecessor_bp + 1) = 0;
    }
    else {
        /* Update predecessor's next_ptr to the successor */
        *((size_t *)predecessor_bp + 1) = (size_t)successor_bp;

        /* Update successor's prev_ptr to the predecessor */
        *((size_t *)successor_bp) = (size_t)predecessor_bp;
    }
}

/* Coalesce the adjacent blocks. There're 4 Cases here. We remove the current */
/* block and possible left and right blocks from their size class list. Then, */
/* merge the lists and insert back into the corresponding class head based on */
/* the resulting size class it belongs to */
static void *coalesce(void *bp) {
    uint32_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(bp)));
    uint32_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    uint32_t size       = GET_SIZE(HDRP(bp));

    if(prev_alloc && next_alloc) {           /* Case 1 */
        return bp;
    }
    else if(prev_alloc && !next_alloc) {     /* Case 2 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));

        removeList(bp);
        removeList(NEXT_BLKP(bp));

        PUT_WORD(HDRP(bp), PACK(size, 0));
        PUT_WORD(FTRP(bp), PACK(size, 0));

        insertList(bp);
    }
    else if(!prev_alloc && next_alloc) {     /* Case 3 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));

        removeList(bp);
        removeList(PREV_BLKP(bp));

        PUT_WORD(FTRP(bp), PACK(size, 0));
        PUT_WORD(HDRP(PREV_BLKP(bp)), PACK(size, 0));

        bp = PREV_BLKP(bp);
        insertList(bp);
    }
    else {                                   /* Case 4 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))) +
                GET_SIZE(HDRP(PREV_BLKP(bp)));

        removeList(bp);
        removeList(PREV_BLKP(bp));
        removeList(NEXT_BLKP(bp));

        PUT_WORD(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT_WORD(FTRP(NEXT_BLKP(bp)), PACK(size, 0));

        bp = PREV_BLKP(bp);
        insertList(bp);
    }

    return bp;
}

static void *extend_heap(size_t words) {
    char *bp;
    size_t size;

    /* Allocate multiple of DSIZE to maintain alignment */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    /* Initialize free block header/footer and the epilogue header */
    PUT_WORD(HDRP(bp), PACK(size, 0));
    PUT_WORD(FTRP(bp), PACK(size, 0));
    PUT_WORD(HDRP(NEXT_BLKP(bp)), PACK(0, CURRENT_ALLOC));

    /* Build the list for particular Size Class */
    insertList((void *)bp);

    /* Coalesce if the adjacent blocks are free */
    return coalesce((void *)bp);
}

/* Find a block with asize (bytes) within particular Size Class */ 
/* based on the arg (class) */
static void *findInClass(size_t asize, int class) {
    void *bp = NULL;

    switch(class) {
        case 1:
            bp = (void *)ptr1;
            break; 
        case 2:
            bp = (void *)ptr2;
            break;
        case 3:
            bp = (void *)ptr3;
            break;
        case 4:
            bp = (void *)ptr4;
            break;
        case 5:
            bp = (void *)ptr5;
            break;
        case 6:
            bp = (void *)ptr6;
            break;
        case 7:
            bp = (void *)ptr7;
            break;
        case 8:
            bp = (void *)ptr8;
            break;
        case 9:
            bp = (void *)ptr9;
            break;
        case 10:
            bp = (void *)ptr10;
            break;
        case 11:
            bp = (void *)ptr11;
            break;
        case 12:
            bp = (void *)ptr12;
            break;
        case 13:
            bp = (void *)ptr13;
        break;
    }

    while(bp != NULL) {
        if(GET_SIZE(HDRP(bp)) >= asize)
            break;
        bp = (void *)SUCCESSOR(bp);
    }

    return bp;
}

/* Find the block larger than asize (bytes) from the lowest size class to */ 
/* the highest. Return the bp of the block if found. Otherwise, return NULL */
static void *find_fit(size_t asize) {
    void *bp;
    int i = 0;
    if(asize <= SEG_CLASS_1) {
        for(i = 1; i <= N_SIZE_CLASS; i++) {
            bp = findInClass(asize, i);
            if(bp != NULL)
                break;
        }
    }
    else if(asize <= SEG_CLASS_2) {
        for(i = 2; i <= N_SIZE_CLASS; i++) {
            bp = findInClass(asize, i);
            if(bp != NULL)
                break;
        }
    }
    else if(asize <= SEG_CLASS_3) {
        for(i = 3; i <= N_SIZE_CLASS; i++) {
            bp = findInClass(asize, i);
            if(bp != NULL)
                break;
        }
    }
    else if(asize <= SEG_CLASS_4) {
        for(i = 4; i <= N_SIZE_CLASS; i++) {
            bp = findInClass(asize, i);
            if(bp != NULL)
                break;
        }
    }
    else if(asize <= SEG_CLASS_5) {
        for(i = 5; i <= N_SIZE_CLASS; i++) {
            bp = findInClass(asize, i);
            if(bp != NULL)
                break;
        }
    }
    else if(asize <= SEG_CLASS_6) {
        for(i = 6; i <= N_SIZE_CLASS; i++) {
            bp = findInClass(asize, i);
            if(bp != NULL)
                break;
        }
    }
    else if(asize <= SEG_CLASS_7) {
        for(i = 7; i <= N_SIZE_CLASS; i++) {
            bp = findInClass(asize, i);
            if(bp != NULL)
                break;
        }
    }
    else if(asize <= SEG_CLASS_8) {
        for(i = 8; i <= N_SIZE_CLASS; i++) {
            bp = findInClass(asize, i);
            if(bp != NULL)
                break;
        }
    }
    else if(asize <= SEG_CLASS_9) {
        for(i = 9; i <= N_SIZE_CLASS; i++) {
            bp = findInClass(asize, i);
            if(bp != NULL)
                break;
        }
    }
    else if(asize <= SEG_CLASS_10) {
        for(i = 10; i <= N_SIZE_CLASS; i++) {
            bp = findInClass(asize, i);
            if(bp != NULL)
                break;
        }
    }
    else if(asize <= SEG_CLASS_11) {
        for(i = 11; i <= N_SIZE_CLASS; i++) {
            bp = findInClass(asize, i);
            if(bp != NULL)
                break;
        }
    }
    else if(asize <= SEG_CLASS_12) {
        for(i = 12; i <= N_SIZE_CLASS; i++) {
            bp = findInClass(asize, i);
            if(bp != NULL)
                break;
        }
    }
    else {
        for(i = 13; i <= N_SIZE_CLASS; i++) {
            bp = findInClass(asize, i);
            if(bp != NULL)
                break;
        }
    }

    return bp;
}

/* Allocate asize bytes of memory from Ptr bp to the user. If the remaining */
/* size is larger than the MINI_BLOCK, we insert the remaining block back to */
/* the list. Otherwise, just give them all */
static void place(void *bp, size_t asize) {
    size_t csize = GET_SIZE(HDRP(bp));
    size_t remaining_size = csize - asize;

    /* remove the block at bp from its size class list */
    removeList(bp);

    if(remaining_size >= MINI_BLOCK) {
        /* Update the asize block */
        PUT_WORD(HDRP(bp), PACK(asize, CURRENT_ALLOC));
        PUT_WORD(FTRP(bp), PACK(asize, CURRENT_ALLOC));

        bp = NEXT_BLKP(bp);
        PUT_WORD(HDRP(bp), PACK(remaining_size, 0));
        PUT_WORD(FTRP(bp), PACK(remaining_size, 0));

        insertList(bp);
    }
    else {
        PUT_WORD(HDRP(bp), PACK(csize, CURRENT_ALLOC));
        PUT_WORD(HDRP(bp), PACK(csize, CURRENT_ALLOC));
    }

    return;
}

/*
 *  End Self defined helper Functions by Chih-Ang
 */

/*
 *  Malloc Implementation
 *  ---------------------
 *  The following functions deal with the user-facing malloc implementation.
 */

/*
 * Initialize: return -1 on error, 0 on success.
 */
int mm_init(void) {

    ptr1 = NULL;        /* Pointer to first block in Class 1 */ 
    ptr2 = NULL;        /* Pointer to first block in Class 2 */
    ptr3 = NULL;        /* Pointer to first block in Class 3 */
    ptr4 = NULL;        /* Pointer to first block in Class 4 */
    ptr5 = NULL;        /* Pointer to first block in Class 5 */
    ptr6 = NULL;        /* Pointer to first block in Class 6 */
    ptr7 = NULL;        /* Pointer to first block in Class 7 */
    ptr8 = NULL;        /* Pointer to first block in Class 8 */
    ptr9 = NULL;        /* Pointer to first block in Class 9 */
    ptr10 = NULL;       /* Pointer to first block in Class 10 */
    ptr11 = NULL;       /* Pointer to first block in Class 11 */
    ptr12 = NULL;       /* Pointer to first block in Class 12 */
    ptr13 = NULL;       /* Pointer to first block in Class 13 */

    /* Create the initial empty heap */
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
       return -1;    
    /* Alignment padding */
    PUT_WORD(heap_listp, 0);
    /* Prologue header */                                        
    PUT_WORD(heap_listp + 1, PACK(DSIZE, CURRENT_ALLOC));
    /* Prologue footer */  
    PUT_WORD(heap_listp + 2, PACK(DSIZE, CURRENT_ALLOC));
    /* Epilogue header */   
    PUT_WORD(heap_listp + 3, PACK(0, CURRENT_ALLOC));

    heap_listp += 2;

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
    if (size <= 2*DSIZE)                                         
       asize = MINI_BLOCK;                                        
    else
    /* Force asize to be rounded (up) to a multiple of DSIZE in bytes */
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
    extendsize = MAX(asize, CHUNKSIZE);
    if((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    if (!aligned(bp))
        printf("Not aligned (Not Found)\n");
    place(bp, asize);
    checkheap(1);      // Let's make sure the heap is ok!
    return bp;
}

/*
 * Free the block pointed to by ptr and insert it back in to the list based on
 * its size class
 */
void free (void *ptr) {
    if (ptr == NULL) {
        return;
    }
    /* check if ptr was retruned by an eariler call to malloc-family */
//    if (FTRP(ptr) != (void *)((char *)ptr + GET_SIZE(HDRP(ptr)) - DSIZE)) {
//        printf("Error: %p is illegal pointer\n", ptr);
//        return;
//    }
    /* update the header and footer of the block's a/f bit */
    size_t size = GET_SIZE(HDRP(ptr));
    PUT_WORD(HDRP(ptr), PACK(size, 0));
    PUT_WORD(FTRP(ptr), PACK(size, 0));

    /* insert the block back into the list */
    insertList(ptr);
    coalesce(ptr);
}

/*
 * realloc - you may want to look at mm-naive.c
 */
void *realloc(void *oldptr, size_t size) {
    size_t oldsize;
    void *newptr;

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

    /* Copy the old data. NOTE: without HEADER and FOOTER */
    oldsize = GET_SIZE(HDRP(oldptr)) - WSIZE;
    oldsize = MIN(size, oldsize);
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

// Returns 0 if no errors were found, otherwise returns the error
int mm_checkheap(int verbose) {
    verbose = verbose;
    return 0;
}
