#include <stdlib.h>  
#include <stdio.h>
#include <linux/unistd.h>
#include <syscall.h>
#include <unistd.h>
#include <stdint.h>
#include <syscall.h>

int __syscall1( int number, intptr_t p1 ){
    int ret;
    asm volatile ("int $0x80" : "=a" (ret) : "a" (number), "b" (p1));
    return ret;
}

void* __sbrk__(intptr_t increment)
{
    //void *new, *old = (void *)syscall(__NR_brk, 0);
    void *new, *old = (void *)__syscall1(__NR_brk, 0);

    //new = (void *)syscall(__NR_brk, ((uintptr_t)old) + increment);
    new = (void *)__syscall1(__NR_brk, ((uintptr_t)old) + increment);

    return (((uintptr_t)new) == (((uintptr_t)old) + increment)) ? old :
        (void *)-1;
}

typedef long Align;  
  
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
        base.s.ptr = freep = prevp = &base;  
        base.s.size = 0;  
    }  
  
    for (p = prevp->s.ptr; ; prevp = p, p = p->s.ptr) {  
  
        if (p->s.size >= nunits) {  
            if (p->s.size == nunits) {  
                prevp->s.ptr = p->s.ptr;  
            } else {  
                p->s.size -= nunits; // allocate the tail of the list. Calculate the difference between p->s.size and nunits then increase the p pointer by this diffference.
                p += p->s.size;  
                p->s.size = nunits;  
            }  
            freep = prevp;  
            return (void *)(p+1); // The bolck containes the header and free space. And p+1 means that p starts to point to free space (after header) 
        }  
  
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
