// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};
struct spinlock page_ref_lock;
int page_ref_cnt[32768]; //128*1024*1024/4096
struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    kfree(p);
  }
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");
  acquire(&page_ref_lock);
  if (page_ref_cnt[((uint64)pa/4096)%32768]<0){
    release(&page_ref_lock);
    return;
  }
  if (page_ref_cnt[((uint64)pa/4096)%32768]>0){
    page_ref_cnt[((uint64)pa/4096)%32768]--;
    if(page_ref_cnt[((uint64)pa/4096)%32768]>0){
      release(&page_ref_lock);
      return;
    }
  }
  // Fill with junk to catch dangling refs.
  memset(pa, 5, PGSIZE);

  r = (struct run*)pa;
  release(&page_ref_lock);
  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 0, PGSIZE); // fill with junk
  if(r && (uint64)r>=KERNBASE && (uint64)r<PHYSTOP){
    acquire(&page_ref_lock);
    page_ref_cnt[((uint64)r/4096)%32768]=1;
    release(&page_ref_lock);
  }
  return (void*)r;
}
