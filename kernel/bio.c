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

#define BUFBUCKETS 13

struct {
  struct spinlock locks[BUFBUCKETS];
 // struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf heads[BUFBUCKETS];
} bcache;

void
binit(void)
{
  //initlock(&bcache.lock,"bcache");
  struct buf *b;

  for(int i=0;i<BUFBUCKETS;i++){
    initlock(&bcache.locks[i], "bcache");

  // Create linked list of buffers
    bcache.heads[i].prev = &bcache.heads[i];
    bcache.heads[i].next = &bcache.heads[i];
  }
  for(b=bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.heads[0].next;
    b->prev = &bcache.heads[0];
    initsleeplock(&b->lock, "buffer");
    bcache.heads[0].next->prev = b;
    bcache.heads[0].next = b;
  }

}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int id=blockno%BUFBUCKETS;
  acquire(&bcache.locks[id]);

  // Is the block already cached?
  for(b = bcache.heads[id].next; b != &bcache.heads[id]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.locks[id]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(int i=(id+1)%BUFBUCKETS;i!=id;i=(i+1)%BUFBUCKETS){
    acquire(&bcache.locks[i]);
    for(b = bcache.heads[i].prev; b != &bcache.heads[i]; b = b->prev){
      if(b->refcnt == 0) {
        b->prev->next=b->next;
        b->next->prev=b->prev;
        release(&bcache.locks[i]);
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;

        b->next = bcache.heads[id].next;
        b->prev = &bcache.heads[id];
        bcache.heads[id].next->prev = b;
        bcache.heads[id].next = b;

        release(&bcache.locks[id]);
        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bcache.locks[i]);

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
  int id=b->blockno%BUFBUCKETS;
  releasesleep(&b->lock);

  acquire(&bcache.locks[id]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.heads[id].next;
    b->prev = &bcache.heads[id];
    bcache.heads[id].next->prev = b;
    bcache.heads[id].next = b;
  }
  
  release(&bcache.locks[id]);
}

void
bpin(struct buf *b) {
  int id=b->blockno%BUFBUCKETS;
  acquire(&bcache.locks[id]);
  b->refcnt++;
  release(&bcache.locks[id]);
}

void
bunpin(struct buf *b) {
  int id=b->blockno%BUFBUCKETS;
  acquire(&bcache.locks[id]);
  b->refcnt--;
  release(&bcache.locks[id]);
}


