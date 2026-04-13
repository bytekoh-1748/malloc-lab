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

/*macros*/
#define PACK(size, alloc) (size|alloc)
#define PUT(p, val) (*(size_t*)(p) = (val))

#define GET_SIZE(ptr) (*ptr & ~0x7)
#define GET_ALLOCATED(ptr) (*ptr & 0x1)

#define GET_PAYLOAD(ptr) ((char*)(ptr) + SIZE_T_SIZE)//이때 ptr은 헤더에서 출발
#define GET_FOOTER(ptr, size) ((char*)(ptr) + SIZE_T_SIZE + size) //이때 ptr은 헤더에서 출발
#define GET_FULLSIZE(size) (SIZE_T_SIZE + size + SIZE_T_SIZE)//이때 ptr은 헤더에서 출발

#define GO_TO_HEADER_FROM_FOOTER(ptr) ((char*)(ptr)-GET_SIZE(ptr)-SIZE_T_SIZE)


static char* headerPtr;
size_t min(size_t a, size_t b);
/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    headerPtr = mem_sbrk(SIZE_T_SIZE * 4);
    if (headerPtr == (void *)-1){return -1;}

    PUT(headerPtr, PACK(0, 1)); //시작 방어용
    PUT(headerPtr + SIZE_T_SIZE, PACK(0, 0)); //진짜 free 헤더
    PUT(headerPtr + SIZE_T_SIZE * 2, PACK(0, 0)); //진짜 free 풋터
    PUT(headerPtr + SIZE_T_SIZE * 3, PACK(0, 1)); //끝 방어용

    headerPtr += SIZE_T_SIZE;
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
    size_t* p = (size_t*)headerPtr;

    while (GET_SIZE(p) != 0)
    {
        if (GET_SIZE(p) >= newSize && !GET_ALLOCATED(p)) {break;}
        p = (size_t*)((char*)p + GET_SIZE(p) + SIZE_T_SIZE + SIZE_T_SIZE); //헤더 하나 풋터 하나
    }

    if(GET_SIZE(p) == 0)
    {
        if (mem_sbrk(GET_FULLSIZE(newSize)) == (void *)-1) return NULL; //에필로그 방어

        PUT(p, PACK(newSize, 1));
        PUT(GET_FOOTER(p,newSize), PACK(newSize,1));

        PUT((char*)GET_FOOTER(p,newSize) + SIZE_T_SIZE, PACK(0,1));
        return GET_PAYLOAD(p);
    }
    else
    {
        size_t oldSize = GET_SIZE(p);
        
        //split 로직
        size_t* newHeader = (size_t*)((char*)GET_FOOTER(p,newSize) + SIZE_T_SIZE);
        if (oldSize >= newSize + 2 * SIZE_T_SIZE + ALIGNMENT) //최소 사이즈를 어떻게 해야할지 모르겠음
        {
            size_t remainPayload = oldSize-newSize-SIZE_T_SIZE * 2;
            PUT(p, PACK(newSize, 1));
            PUT(GET_FOOTER(p,newSize), PACK(newSize,1));
            PUT(newHeader, PACK(remainPayload, 0));
            PUT(GET_FOOTER(newHeader, remainPayload), PACK(remainPayload, 0));
        }
        else
        {
            PUT(p, PACK(oldSize,1));
            PUT(GET_FOOTER(p, oldSize), PACK(oldSize, 1));
        }
        return GET_PAYLOAD(p);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    if (ptr == NULL) {return;}
    size_t* ptrHead = (size_t*)((char*)ptr - SIZE_T_SIZE);

    size_t* prevFooter = (size_t*)((char*)ptrHead - SIZE_T_SIZE);
    size_t* nextHeader = (size_t*)(GET_FOOTER(ptrHead, GET_SIZE(ptrHead)) + SIZE_T_SIZE);


    if (GET_ALLOCATED(prevFooter) != 1 && GET_ALLOCATED(nextHeader)!= 1)
    {
        size_t newSize = GET_SIZE(prevFooter) + GET_SIZE(ptrHead) + GET_SIZE(nextHeader) + SIZE_T_SIZE * 4;
        PUT(GO_TO_HEADER_FROM_FOOTER(prevFooter), PACK(newSize, 0));
        PUT(GET_FOOTER(nextHeader, GET_SIZE(nextHeader)), PACK(newSize, 0));

    }
    else if (GET_ALLOCATED(prevFooter) != 1 && GET_ALLOCATED(nextHeader) == 1)
    {
        size_t newSize = GET_SIZE(prevFooter) + GET_SIZE(ptrHead) + 2 * SIZE_T_SIZE;
        PUT(GO_TO_HEADER_FROM_FOOTER(prevFooter), PACK(newSize, 0));
        PUT(GET_FOOTER(ptrHead, GET_SIZE(ptrHead)), PACK(newSize, 0));
    }
    else if (GET_ALLOCATED(prevFooter) == 1 && GET_ALLOCATED(nextHeader) != 1)
    {
        size_t newSize = GET_SIZE(nextHeader) + GET_SIZE(ptrHead) + 2 * SIZE_T_SIZE;
        PUT(ptrHead, PACK(newSize, 0));
        PUT(GET_FOOTER(nextHeader, GET_SIZE(nextHeader)), PACK(newSize, 0));
    }
    else
    {
        PUT(ptrHead, PACK(GET_SIZE(ptrHead), 0)); //size로 allocate 초기화
        PUT(GET_FOOTER(ptrHead, GET_SIZE(ptrHead)), PACK(GET_SIZE(ptrHead), 0));
    }
    
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    if (ptr == NULL) {return mm_malloc(size);}
    if (size == 0) {mm_free(ptr); return NULL;}
    size_t* oldPtrHead = (size_t*)((char*)ptr - SIZE_T_SIZE);
    size_t* newPtrPayload = mm_malloc(size);
    if (newPtrPayload == NULL) {return NULL;}
    //copy 해야함
    memcpy(newPtrPayload, ptr, min(size, GET_SIZE(oldPtrHead)));
    mm_free(ptr);
    return newPtrPayload;
}

size_t min(size_t a, size_t b)
{
    return (a<b) ? a : b;
}