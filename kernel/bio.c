// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"
#define BUCKETSIZE 17
struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head;
} bcache;

struct {
  struct spinlock lock;
  struct buf buf[NBUF / BUCKETSIZE + 1];
  struct buf head;
  char name[16];
} bcache_n[BUCKETSIZE];

void
binit(void)
{
  struct buf *b;

  for(int i = 0; i < BUCKETSIZE; i++){
    if(snprintf(bcache_n[i].name, 16, "bcache%d", i) <= 0)
      panic("binit\n");
    initlock(&bcache_n[i].lock, bcache_n[i].name);
  }
  // initlock(&bcache.lock, "bcache");

  // Create linked list of buffers
  for(int i = 0; i < BUCKETSIZE; i++){
    bcache_n[i].head.prev = &bcache_n[i].head;
    bcache_n[i].head.next = &bcache_n[i].head;
    for(b = bcache_n[i].buf; b < bcache_n[i].buf+NBUF / BUCKETSIZE + 1; b++){
      b->next = bcache_n[i].head.next;
      b->prev = &bcache_n[i].head;
      initsleeplock(&b->lock, "buffer");
      bcache_n[i].head.next->prev = b;
      bcache_n[i].head.next = b;
    }
  }
}

uint get_hash_index(uint blockno){
  return blockno % BUCKETSIZE;
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  uint i = get_hash_index(blockno);

  acquire(&bcache_n[i].lock);

  // Is the block already cached?
  for(b = bcache_n[i].head.next; b != &bcache_n[i].head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache_n[i].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  release(&bcache_n[i].lock);
  for(int j = 0; j < BUCKETSIZE; j++){
    if(i < j){
      acquire(&bcache_n[i].lock);
      acquire(&bcache_n[j].lock);
    } else if(i > j){
      acquire(&bcache_n[j].lock);
      acquire(&bcache_n[i].lock);
    } else{
      acquire(&bcache_n[i].lock);
    }

    for(b = bcache_n[j].head.prev; b != &bcache_n[j].head; b = b->prev){
      if(b->refcnt == 0) {
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        //remove it from ori bucket
        b->next->prev = b->prev;
        b->prev->next = b->next;

        //insert it into new bucket
        b->next = bcache_n[i].head.next;
        b->prev = &bcache_n[i].head;
        bcache_n[i].head.next->prev = b;
        bcache_n[i].head.next = b;

        release(&bcache_n[i].lock);
        if(i != j)
          release(&bcache_n[j].lock);
        acquiresleep(&b->lock);

        return b;
      }
    }

    release(&bcache_n[i].lock);
    if(i != j)
      release(&bcache_n[j].lock);
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");
  // uint i = get_hash_index(b->blockno);
  releasesleep(&b->lock);

  // acquire(&bcache_n[i].lock);
  b->refcnt--;
  // if (b->refcnt == 0) {
  //   // no one is waiting for it.
  //   b->next->prev = b->prev;
  //   b->prev->next = b->next;
  //   b->next = bcache_n[i].head.next;
  //   b->prev = &bcache_n[i].head;
  //   bcache_n[i].head.next->prev = b;
  //   bcache_n[i].head.next = b;
  // }
  
  // release(&bcache_n[i].lock);
}

void
bpin(struct buf *b) {
  uint i = get_hash_index(b->blockno);
  acquire(&bcache_n[i].lock);
  b->refcnt++;
  release(&bcache_n[i].lock);
}

void
bunpin(struct buf *b) {
  uint i = get_hash_index(b->blockno);
  acquire(&bcache_n[i].lock);
  b->refcnt--;
  release(&bcache_n[i].lock);
}


