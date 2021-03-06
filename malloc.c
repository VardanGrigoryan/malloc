
#include <linux/unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>  
#include <syscall.h>
#include <unistd.h>

struct max_align_t {
    long long __max_align_ll __attribute__((__aligned__(__alignof__(long long))));
    long double __max_align_ld __attribute__((__aligned__(__alignof__(long double))));
} ;

int __syscall1( int number, intptr_t p1 ){
    int ret;
    asm volatile ("int $0x80" : "=a" (ret) : "a" (number), "b" (p1));
    return ret;
}

void* __sbrk__(intptr_t increment)
{
    /*
     * Calling sbrk() with an increment of 0 can be used to find the current 
     * location of the program break.
     * The program break is the first location after the end of the 
     * uninitialized data segment. Program break means the top of the Heap). 
     * Increasing the program break has the effect of allocating memory 
     * to the process; decreasing the break deallocates memory.
      */
    void *new, *old = (void *)__syscall1(__NR_brk, 0);
    new = (void *)__syscall1(__NR_brk, ((uintptr_t)old) + increment);
    return (((uintptr_t)new) == (((uintptr_t)old) + increment)) ? old :
        (void *)-1;
}

typedef struct max_align_t Align;  
  
union header {  
    struct {  
         union header *ptr;  
         unsigned size;  
    } s;  
    Align x;  
};  
  
typedef union header Header;  
  
static Header base;  
static Header *freep = NULL;  
  
#define NALLOC 1024  
  
void _free(void *ap)  
{  
    Header *bp, *p;  
  
    bp = (Header *)ap - 1;  
  
    for (p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr)  
         if (p >= p->s.ptr && (bp > p || bp < p->s.ptr))  
             break;  
  
    if (bp + bp->s.size == p->s.ptr) {  
        bp->s.size += p->s.ptr->s.size;  
        bp->s.ptr = p->s.ptr->s.ptr;  
    } else {  
        bp->s.ptr = p->s.ptr;  
    }  
  
    if (p + p->s.size == bp) {  
        p->s.size += bp->s.size;  
        p->s.ptr = bp->s.ptr;  
    } else {  
        p->s.ptr = bp; 
    }  
  
    freep = p;  
}

static Header *morecore(unsigned nu)  
{  
    char *cp;  
    Header *up;  
  
    if (nu < NALLOC)  
        nu = NALLOC;  
  
    cp = (char*)__sbrk__(nu * sizeof(Header));  
    if (cp == (char *) -1)  
        return NULL;  
  
    up = (Header *) cp;  
    up->s.size = nu;  
    _free((void *)(up + 1));  // The bolck containes the header and free space. And up + 1 means performing _free only for free space (free space comes after header, the header does not change) 
  
    return freep;  
}  
  
void *_malloc(unsigned nbytes)  
{  
    Header *p, *prevp;  
    unsigned nunits;  
  
    nunits = (nbytes + sizeof(Header) - 1) / sizeof(Header) + 1;  
  
    if ((prevp = freep) == NULL) {  
        /*
         * The base variable is the global, static variable therefor all its data-member are initialized to 0 by default.
         * The freep variable means the head or start of the free-list and because free-list is cyrcular, each time updated.
         * */
        base.s.ptr = freep = prevp = &base;  // starting initilializations
        base.s.size = 0;  
    }  
    /*
     * keeps the previous pointer prep and iterates through free-list if there is no big-enough block and continues prior to accomplish the end of the free-list.
     * if there is big-enough block and when its size equals to requested size, the next pointer of the prevp starts to point to the next block after p pointer. Thereby it removes this block and return
     * pointer after the head of this block.
     * */
    for (p = prevp->s.ptr; ; prevp = p, p = p->s.ptr) {  
  
        if (p->s.size >= nunits) {  
            if (p->s.size == nunits) {  
                prevp->s.ptr = p->s.ptr;  
            } else { 
                /*
                 * cuts the tail of the block
                 * */
                p->s.size -= nunits; // allocate the tail of the list. Calculate the difference between p->s.size and nunits then increase the p pointer by this diffference.
                p += p->s.size;  
                p->s.size = nunits;  
            }  
            freep = prevp; // update the start(head) of the free-list, now the prevp starts to point to freep and freep now is the start of the free-list. 
            return (void *)(p+1); // The bolck containes the header and free space. And p+1 means that p starts to point to free space (after header) 
        } 
        /*
         * if it is the end of the free-list and there is not a memory, it requests to os to allocate the block of the memory.
         * p == freep means that is reached to the end of the free-list.
         * */
  
        if (p == freep)  
            if ((p = morecore(nunits)) == NULL)  
                return NULL;  
    }  
}  
 
int main()
{
    char* p = (char*) _malloc(6);
    p[0] = 'v';
    p[1] = 'a';
    p[2] = 'r';
    p[3] = 'd';
    p[4] = 'a';
    p[5] = 'n';
    int i = 0;
    for(; i < 6; ++i) {
        printf("%c\n", p[i]);
    }
}
