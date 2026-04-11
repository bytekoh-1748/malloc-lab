/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* Block */

typedef struct _impblock{
    size_t size;
    int allocated;
} ImpBlock;

ImpBlock* starter;

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    starter = NULL;
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    if (starter== NULL)
    {
        int newsize = ALIGN(size + sizeof(ImpBlock));
        void *p = mem_sbrk(newsize);
        if (p == (void *)-1)
            return NULL;
        starter = (ImpBlock*)p;
        starter->size = newsize;
        starter->allocated = 1;
    
        return (void *)((char *)p + sizeof(ImpBlock));

    }

    else
    {
        size_t newsize = ALIGN(size + sizeof(ImpBlock));
        ImpBlock* ptr = starter;

        while ((char*)ptr <= (char*)mem_heap_hi())
        {
            if (ptr->allocated == 0 && ptr->size >= newsize)
            {
                ptr->allocated = 1;
                return (void*)((char*)ptr + sizeof(ImpBlock));
            }

            ptr = (ImpBlock*)((char*)ptr + ptr->size);
        }
 
        void *p = mem_sbrk(newsize);
        if (p == (void *)-1)
        return NULL;
        ImpBlock* newBlock = p;
        newBlock->allocated = 1;
        newBlock->size = newsize;

        return (void *)((char *)p + sizeof(ImpBlock));
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    if (ptr == NULL) {return;}
    else
    {
        ImpBlock* temp = (ImpBlock*)((char*) ptr - sizeof(ImpBlock));
        temp->allocated = 0;
        return;
    }

}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    if (ptr == NULL) {return mm_malloc(size);}
    if (size == 0) {return NULL;}

    void *oldptr = ptr;
    void *newptr;
    ImpBlock* copySize;
    newptr = mm_malloc(size);


    if (newptr == NULL)
        return NULL;
    copySize = (ImpBlock*)(((char*)oldptr - sizeof(ImpBlock)));
    if (copySize->size - sizeof(ImpBlock) > size){memcpy(newptr, oldptr, size);}
    else {memcpy(newptr, oldptr, copySize->size - sizeof(ImpBlock));}
    mm_free(oldptr);
    return newptr;
}