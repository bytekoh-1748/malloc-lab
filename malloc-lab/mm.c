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
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/*macros*/
#define MINIMUM_SIZE (SIZE_T_SIZE * 4) //header + prev + next + footer


#define PUT(ptr, val) *(size_t*)(ptr) = (val)
#define PACK(size, alloc) ((size) | (alloc)) //size는 payload 크기

#define GO_TO_FOOTER(header) ((size_t*)((char*)(header) + GET_SIZE(header)+ SIZE_T_SIZE))
#define NEXT_BLOCK(header) (size_t*)(((char*)(header) + SIZE_T_SIZE + GET_SIZE(header) + SIZE_T_SIZE)) //free,allocated 상관없이
#define GO_TO_PREV_FOOTER(header) (size_t*)((char*)(header) - SIZE_T_SIZE)
#define GO_TO_HEADER(footer) (size_t*)((char*)footer - SIZE_T_SIZE - GET_SIZE(footer))

#define GET_SIZE(header) (*(size_t*)(header) & ~0x7)
#define GET_ALLOC(header) (*(size_t*)(header)  & 0x7)
#define GET_PREV(header) *(size_t*)(((char*)header + SIZE_T_SIZE))
#define GET_NEXT(header) *(size_t*)(((char*)header + SIZE_T_SIZE * 2))
#define GET_PAYLOAD(header) (size_t*)(((char*)header + SIZE_T_SIZE))
#define GET_HEADER(payloadPtr) (size_t*)(((char*)payloadPtr - SIZE_T_SIZE))

/*Global Variables*/
static size_t* heapHeader;
static size_t* freeHeader;
static size_t* heapFooter;


size_t min(size_t a, size_t b);


/* Explicit Free List*/
/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    size_t* p = mem_sbrk(MINIMUM_SIZE + SIZE_T_SIZE * 3); //padding 생각해서 넉넉히
    if (p == (void*) -1) {return -1;}

    size_t padding = (ALIGNMENT - ((size_t)p % ALIGNMENT)) % ALIGNMENT;
    heapHeader = (size_t*)((char*)p + padding);

    PUT(heapHeader, PACK(0,1));
    size_t* firstFree = NEXT_BLOCK(heapHeader);

    PUT(firstFree, PACK(SIZE_T_SIZE * 2, 0));
    PUT(GO_TO_FOOTER(firstFree), PACK(SIZE_T_SIZE * 2, 0));
    PUT(NEXT_BLOCK(firstFree), PACK(0,1));

    GET_PREV(firstFree) = (size_t)NULL;
    GET_NEXT(firstFree) = (size_t)NULL;

    heapFooter = NEXT_BLOCK(firstFree);
    freeHeader = firstFree;

    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    if (size == 0) {return NULL;}

    size_t newSize = ALIGN(size);
    if (newSize < SIZE_T_SIZE * 2) {newSize = SIZE_T_SIZE * 2;} //최소 메모리 공간 지키기 이떄 size는 payload
    
    size_t* ptr = freeHeader;

    while (ptr != NULL && GET_SIZE(ptr) != 0)
    {
        if (GET_ALLOC(ptr) == 0 && GET_SIZE(ptr) >= newSize){break;}
        ptr = GET_NEXT(ptr);
    }

    if (ptr == NULL || GET_SIZE(ptr) == 0)
    {
        size_t* p = mem_sbrk(newSize + SIZE_T_SIZE * 3);
        if (p == (void*) -1) {return NULL;}

        PUT(p, PACK(newSize, 1));
        PUT(GO_TO_FOOTER(p), PACK(newSize, 1));

        PUT(NEXT_BLOCK(p), PACK(0, 1)); //에필로그 방어
        heapFooter = NEXT_BLOCK(p);

        return GET_PAYLOAD(p);
    }
    else
    {
        size_t* prev = (size_t*)GET_PREV(ptr);
        size_t* next = (size_t*)GET_NEXT(ptr);

        if (GET_SIZE(ptr) - newSize >= MINIMUM_SIZE + SIZE_T_SIZE * 2)
        {
            size_t oldSize = GET_SIZE(ptr);
            PUT(ptr, PACK(newSize, 1));
            PUT(GO_TO_FOOTER(ptr), PACK(newSize, 1));

            PUT(NEXT_BLOCK(ptr), PACK(oldSize - newSize - SIZE_T_SIZE * 2, 0));
            PUT(GO_TO_FOOTER(NEXT_BLOCK(ptr)), PACK(oldSize - newSize - SIZE_T_SIZE * 2, 0));
            GET_PREV(NEXT_BLOCK(ptr)) = prev;
            GET_NEXT(NEXT_BLOCK(ptr)) = next;

            if (prev == NULL) {freeHeader = NEXT_BLOCK(ptr);}
            else {GET_NEXT(prev) = NEXT_BLOCK(ptr);}
            if (next != NULL) {GET_PREV(next) = NEXT_BLOCK(ptr);}

            return GET_PAYLOAD(ptr);
        }
        else
        {   
            if (prev != NULL) {GET_NEXT(prev) = next;}
            else {freeHeader = next;}
            if (next != NULL) {GET_PREV(next) = prev;}

            PUT(ptr, PACK(GET_SIZE(ptr), 1));
            PUT(GO_TO_FOOTER(ptr), PACK(GET_SIZE(ptr), 1));


            return GET_PAYLOAD(ptr);
        }
        //split 없이
        // size_t* prev = (size_t*)GET_PREV(ptr);
        // size_t* next = (size_t*)GET_NEXT(ptr);

        // if (prev != NULL) {GET_NEXT(prev) = next;}
        // else {freeHeader = next;}
        // if (next != NULL) {GET_PREV(next) = prev;}

        // PUT(ptr, PACK(GET_SIZE(ptr), 1));
        // PUT(GO_TO_FOOTER(ptr), PACK(GET_SIZE(ptr), 1));

        // return GET_PAYLOAD(ptr);
    }  

}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{  
    if (ptr == NULL) {return;}

    size_t* ptrHeader = GET_HEADER(ptr);
    if (freeHeader == NULL)
    {
        PUT(ptrHeader, PACK(GET_SIZE(ptrHeader), 0));
        PUT(GO_TO_FOOTER(ptrHeader), PACK(GET_SIZE(ptrHeader), 0));
        GET_PREV(ptrHeader) = (size_t)NULL;
        GET_NEXT(ptrHeader) = (size_t)NULL;

        freeHeader = ptrHeader;
        return;
    } //새로운 freeheader를 지정해줘야한다-그리고 현재 free block이 아예 없다는뜻

    else
    {
        //병합 있을 때
        if (!GET_ALLOC(GO_TO_PREV_FOOTER(ptrHeader)) && !GET_ALLOC(NEXT_BLOCK(ptrHeader)))
        {
            size_t* prevHeader = GO_TO_HEADER(GO_TO_PREV_FOOTER(ptrHeader));
            size_t newSize = GET_SIZE(ptrHeader) + GET_SIZE(prevHeader) + GET_SIZE(NEXT_BLOCK(ptrHeader)) + SIZE_T_SIZE * 4;
            PUT(prevHeader,PACK(newSize, 0));
            PUT(GO_TO_FOOTER(NEXT_BLOCK(ptrHeader)), PACK(newSize, 0));

            GET_NEXT(prevHeader) = GET_NEXT(freeHeader);
            freeHeader = prevHeader;

        }
        else if (!GET_ALLOC(GO_TO_PREV_FOOTER(ptrHeader)) && GET_ALLOC(NEXT_BLOCK(ptrHeader)))
        {
            size_t* prevHeader = GO_TO_HEADER(GO_TO_PREV_FOOTER(ptrHeader));
            size_t newSize = GET_SIZE(ptrHeader) + GET_SIZE(prevHeader) + SIZE_T_SIZE * 2;
            PUT(prevHeader,PACK(newSize, 0));
            PUT(GO_TO_FOOTER(ptrHeader), PACK(newSize, 0));

        }
        else if (GET_ALLOC(GO_TO_PREV_FOOTER(ptrHeader)) && !GET_ALLOC(NEXT_BLOCK(ptrHeader)))
        {
            size_t newSize = GET_SIZE(ptrHeader) + GET_SIZE(NEXT_BLOCK(ptrHeader)) + SIZE_T_SIZE * 2;
            PUT(ptrHeader,PACK(newSize, 0));
            PUT(GO_TO_FOOTER(NEXT_BLOCK(ptrHeader)), PACK(newSize, 0));

            GET_PREV(ptrHeader) = GET_PREV(NEXT_BLOCK(ptrHeader));
            GET_NEXT(ptrHeader) = GET_NEXT(NEXT_BLOCK(ptrHeader));

        }
        else
        {
            PUT(ptrHeader, PACK(GET_SIZE(ptrHeader), 0));
            PUT(GO_TO_FOOTER(ptrHeader), PACK(GET_SIZE(ptrHeader), 0));

            GET_PREV(ptrHeader) = NULL;
            GET_NEXT(ptrHeader) = freeHeader;
            if (freeHeader != NULL) GET_PREV(freeHeader) = ptrHeader;
            freeHeader = ptrHeader;

        }

        //병합 없을 때
        // PUT(ptrHeader, PACK(GET_SIZE(ptrHeader), 0));
        // PUT(GO_TO_FOOTER(ptrHeader), PACK(GET_SIZE(ptrHeader), 0));

        // GET_PREV(ptrHeader) = (size_t)NULL;
        // GET_NEXT(ptrHeader) = (size_t)freeHeader;
        // GET_PREV(freeHeader) = (size_t)ptrHeader;
        // freeHeader = ptrHeader;
    }

} //맨 앞에 집어넣는 방식

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    if (ptr == NULL) {return mm_malloc(size);}
    if (size == 0) {mm_free(ptr); return NULL;}

    void* temp = mm_malloc(size);
    if (temp == NULL){return NULL;}
    else
    {
        memcpy(temp, ptr, min(size, GET_SIZE(GET_HEADER(ptr))));
        mm_free(ptr);
        return temp;
    }

}


size_t min(size_t a, size_t b)
{
    return (a<b) ? a : b;
}

