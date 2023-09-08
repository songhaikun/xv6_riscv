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

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

struct {
  struct spinlock lock;
  struct run *freelist;
  char name[16];
} kmem_n[NCPU];

void
kinit()
{
  //init n kmem lock
  for(int i = 0; i < NCPU; i++){
    if(snprintf(kmem_n[i].name, 16, "kmem%d", i) <= 0)
      panic("kinit\n");
    initlock(&kmem_n[i].lock, kmem_n[i].name); // need change
  }
  // initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);
  int i = ((uint64)pa - (uint64)PGROUNDUP(PHYSTOP)) / PGSIZE % NCPU; // 交错分配
  r = (struct run*)pa;

  acquire(&kmem_n[i].lock);
  r->next = kmem_n[i].freelist;
  kmem_n[i].freelist = r;
  release(&kmem_n[i].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  push_off();
  int cid = cpuid();
  pop_off();

  acquire(&kmem_n[cid].lock);
  r = kmem_n[cid].freelist;
  if(r){
    kmem_n[cid].freelist = r->next;
    release(&kmem_n[cid].lock);
  }
  else{
    release(&kmem_n[cid].lock);
    for(int i = 0; (i < NCPU) && (0 == r); i++){
      if(i == cid)
        continue;
      if(i < cid){
        acquire(&kmem_n[i].lock);
        acquire(&kmem_n[cid].lock); // can remove
      }else{
        acquire(&kmem_n[cid].lock); // can remove
        acquire(&kmem_n[i].lock);
      }
      struct run *r1 = kmem_n[i].freelist;
      if(r1){
        r = r1;
        kmem_n[i].freelist = r1->next;
      }
      release(&kmem_n[i].lock);
      release(&kmem_n[cid].lock); // can remove
    }
  }
  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
