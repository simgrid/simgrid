/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Redefine the classical malloc/free/realloc functions so that they fit well in the mmalloc framework */

#include "mmprivate.h"
#include "gras_config.h"
#include <math.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_mm_legacy, xbt,
                                "Logging specific to mm_legacy in mmalloc");

/* The mmalloc() package can use a single implicit malloc descriptor
   for mmalloc/mrealloc/mfree operations which do not supply an explicit
   descriptor.  This allows mmalloc() to provide
   backwards compatibility with the non-mmap'd version. */
struct mdesc *__mmalloc_default_mdp;


static void *__mmalloc_current_heap = NULL;     /* The heap we are currently using. */

#include "xbt_modinter.h"

void *mmalloc_get_current_heap(void)
{
  return __mmalloc_current_heap;
}

void mmalloc_set_current_heap(void *new_heap)
{
  __mmalloc_current_heap = new_heap;
}

#ifdef MMALLOC_WANT_OVERIDE_LEGACY
void *malloc(size_t n)
{
  void *mdp = __mmalloc_current_heap;
#ifdef HAVE_MMAP
  if (!mdp)
    mmalloc_preinit();
#endif
  LOCK(mdp);
  void *ret = mmalloc(mdp, n);
  UNLOCK(mdp);

  return ret;
}

void *calloc(size_t nmemb, size_t size)
{
  size_t total_size = nmemb * size;
  void *mdp = __mmalloc_current_heap;
#ifdef HAVE_MMAP
  if (!mdp)
    mmalloc_preinit();
#endif
  LOCK(mdp);
  void *ret = mmalloc(mdp, total_size);
  UNLOCK(mdp);

  /* Fill the allocated memory with zeroes to mimic calloc behaviour */
  memset(ret, '\0', total_size);

  return ret;
}

void *realloc(void *p, size_t s)
{
  void *ret = NULL;
  void *mdp = __mmalloc_current_heap;
#ifdef HAVE_MMAP
  if (!mdp)
    mmalloc_preinit();
#endif
  LOCK(mdp);
  if (s) {
    if (p)
      ret = mrealloc(mdp, p, s);
    else
      ret = mmalloc(mdp, s);
  } else {
    mfree(mdp, p);
  }
  UNLOCK(mdp);

  return ret;
}

void free(void *p)
{
  void *mdp = __mmalloc_current_heap;
#ifdef HAVE_GTNETS
  if(!mdp) return;
#endif
  LOCK(mdp);
  mfree(mdp, p);
  UNLOCK(mdp);
}
#endif

/* Make sure it works with md==NULL */

/* Safety gap from the heap's break address.
 * Try to increase this first if you experience strange errors under
 * valgrind. */
#define HEAP_OFFSET   (128UL<<20)

void *mmalloc_get_default_md(void)
{
  xbt_assert(__mmalloc_default_mdp);
  return __mmalloc_default_mdp;
}

static void mmalloc_fork_prepare(void)
{
  struct mdesc* mdp = NULL;
  if ((mdp =__mmalloc_default_mdp)){
    while(mdp){
      LOCK(mdp);
      if(mdp->fd >= 0){
        mdp->refcount++;
      }
      mdp = mdp->next_mdesc;
    }
  }
}

static void mmalloc_fork_parent(void)
{
  struct mdesc* mdp = NULL;
  if ((mdp =__mmalloc_default_mdp)){
    while(mdp){
      if(mdp->fd < 0)
        UNLOCK(mdp);
      mdp = mdp->next_mdesc;
    }
  }
}

static void mmalloc_fork_child(void)
{
  struct mdesc* mdp = NULL;
  if ((mdp =__mmalloc_default_mdp)){
    while(mdp){
      UNLOCK(mdp);
      mdp = mdp->next_mdesc;
    }
  }
}

/* Initialize the default malloc descriptor. */
void mmalloc_preinit(void)
{
  int res;
  if (!__mmalloc_default_mdp) {
    unsigned long mask = ~((unsigned long)getpagesize() - 1);
    void *addr = (void*)(((unsigned long)sbrk(0) + HEAP_OFFSET) & mask);
    __mmalloc_default_mdp = mmalloc_attach(-1, addr);
    /* Fixme? only the default mdp in protected against forks */
    res = xbt_os_thread_atfork(mmalloc_fork_prepare,
			       mmalloc_fork_parent, mmalloc_fork_child);
    if (res != 0)
      THROWF(system_error,0,"xbt_os_thread_atfork() failed: return value %d",res);
  }
  xbt_assert(__mmalloc_default_mdp != NULL);
}

void mmalloc_postexit(void)
{
  /* Do not detach the default mdp or ldl won't be able to free the memory it allocated since we're in memory */
  //  mmalloc_detach(__mmalloc_default_mdp);
  mmalloc_pre_detach(__mmalloc_default_mdp);
}

int mmalloc_compare_heap(void *h1, void *h2, void *std_heap_addr){

  if(h1 == NULL && h2 == NULL){
    XBT_DEBUG("Malloc descriptors null");
    return 0;
  }

  /* Heapstats */

  int errors = 0;

  struct mstats ms1 = mmstats(h1);
  struct mstats ms2 = mmstats(h2);

  if(ms1.chunks_used !=  ms2.chunks_used){
    if(XBT_LOG_ISENABLED(xbt_mm_legacy, xbt_log_priority_debug)){
      XBT_DEBUG("Different chunks allocated by the user : %zu - %zu", ms1.chunks_used, ms2.chunks_used);
      errors++;
    }else{
      return 1;
    }
  }

  if(ms1.bytes_used !=  ms2.bytes_used){
    if(XBT_LOG_ISENABLED(xbt_mm_legacy, xbt_log_priority_debug)){
      XBT_DEBUG("Different byte total of user-allocated chunks : %zu - %zu", ms1.bytes_used, ms2.bytes_used);
      errors++;
    }else{
      return 1;
    }
  }

  if(ms1.bytes_free !=  ms2.bytes_free){
    if(XBT_LOG_ISENABLED(xbt_mm_legacy, xbt_log_priority_debug)){
      XBT_DEBUG("Different byte total of chunks in the free list : %zu - %zu", ms1.bytes_free, ms2.bytes_free);
      errors++;
    }else{
      return 1;
    }
  }

  if(ms1.chunks_free !=  ms2.chunks_free){
    if(XBT_LOG_ISENABLED(xbt_mm_legacy, xbt_log_priority_debug)){
      XBT_DEBUG("Different chunks in the free list : %zu - %zu", ms1.chunks_free, ms2.chunks_free);
      errors++;
    }else{
      return 1;
    }
  }

  struct mdesc *mdp1, *mdp2;
  mdp1 = MD_TO_MDP(h1);
  mdp2 = MD_TO_MDP(h2);

  int res = mmalloc_compare_mdesc(mdp1, mdp2, std_heap_addr);

  return ((errors + res ) > 0);

}

int mmalloc_compare_mdesc(struct mdesc *mdp1, struct mdesc *mdp2, void *std_heap_addr){

  int errors = 0;

  if(mdp1->headersize != mdp2->headersize){
    if(XBT_LOG_ISENABLED(xbt_mm_legacy, xbt_log_priority_debug)){
      XBT_DEBUG("Different size of the file header for the mapped files");
      errors++;
    }else{
      return 1;
    }
  }

  if(mdp1->refcount != mdp2->refcount){
    if(XBT_LOG_ISENABLED(xbt_mm_legacy, xbt_log_priority_debug)){
      XBT_DEBUG("Different number of processes that attached the heap");
      errors++;
    }else{
      return 1;
    }
  }
 
  if(strcmp(mdp1->magic, mdp2->magic) != 0){
    if(XBT_LOG_ISENABLED(xbt_mm_legacy, xbt_log_priority_debug)){
      XBT_DEBUG("Different magic number");
      errors++;
    }else{
      return 1;
    }
  }

  if(mdp1->flags != mdp2->flags){
    if(XBT_LOG_ISENABLED(xbt_mm_legacy, xbt_log_priority_debug)){
      XBT_DEBUG("Different flags");
      errors++;
    }else{
      return 1;
    }
  }

  if(mdp1->heapsize != mdp2->heapsize){
    if(XBT_LOG_ISENABLED(xbt_mm_legacy, xbt_log_priority_debug)){
      XBT_DEBUG("Different number of info entries");
      errors++;
    }else{
      return 1;
    }
  }

  //XBT_DEBUG("Heap size : %zu", mdp1->heapsize);

  if(mdp1->heapbase != mdp2->heapbase){
    if(XBT_LOG_ISENABLED(xbt_mm_legacy, xbt_log_priority_debug)){
      XBT_DEBUG("Different first block of the heap");
      errors++;
    }else{
      return 1;
    }
  }


  if(mdp1->heapindex != mdp2->heapindex){
    if(XBT_LOG_ISENABLED(xbt_mm_legacy, xbt_log_priority_debug)){
      XBT_DEBUG("Different index for the heap table : %zu - %zu", mdp1->heapindex, mdp2->heapindex);
      errors++;
    }else{
      return 1;
    }
  }

  //XBT_DEBUG("Heap index : %zu", mdp1->heapindex);

  if(mdp1->base != mdp2->base){
    if(XBT_LOG_ISENABLED(xbt_mm_legacy, xbt_log_priority_debug)){
      XBT_DEBUG("Different base address of the memory region");
      errors++;
    }else{
      return 1;
    }
  }

  if(mdp1->breakval != mdp2->breakval){
    if(XBT_LOG_ISENABLED(xbt_mm_legacy, xbt_log_priority_debug)){
      XBT_DEBUG("Different current location in the memory region");
      errors++;
    }else{
      return 1;
    }
  }

  if(mdp1->top != mdp2->top){
    if(XBT_LOG_ISENABLED(xbt_mm_legacy, xbt_log_priority_debug)){
      XBT_DEBUG("Different end of the current location in the memory region");
      errors++;
    }else{
      return 1;
    }
  }
  
  if(mdp1->heaplimit != mdp2->heaplimit){
    if(XBT_LOG_ISENABLED(xbt_mm_legacy, xbt_log_priority_debug)){
      XBT_DEBUG("Different limit of valid info table indices");
      errors++;
    }else{
      return 1;
    }
  }

  if(mdp1->fd != mdp2->fd){
    if(XBT_LOG_ISENABLED(xbt_mm_legacy, xbt_log_priority_debug)){
      XBT_DEBUG("Different file descriptor for the file to which this malloc heap is mapped");
      errors++;
    }else{
      return 1;
    }
  }

   if(mdp1->version != mdp2->version){
    if(XBT_LOG_ISENABLED(xbt_mm_legacy, xbt_log_priority_debug)){
      XBT_DEBUG("Different version of the mmalloc package");
      errors++;
    }else{
      return 1;
    }
  }

 
  size_t block_free1, block_free2 , next_block_free, first_block_free, block_free ;
  size_t i, j;
  void *addr_block1, *addr_block2;
  size_t frag_size;
 

  /* Search index of the first free block */

  block_free1 = mdp1->heapindex; 
  block_free2 = mdp2->heapindex;

  while(mdp1->heapinfo[block_free1].free.prev != 0){
    block_free1 = mdp1->heapinfo[block_free1].free.prev;
  }

  while(mdp2->heapinfo[block_free2].free.prev != 0){
    block_free2 = mdp1->heapinfo[block_free2].free.prev;
  }

  if(block_free1 !=  block_free2){
    if(XBT_LOG_ISENABLED(xbt_mm_legacy, xbt_log_priority_debug)){
      XBT_DEBUG("Different first free block");
      errors++;
    }else{
      return 1;
    }
  }

  first_block_free = block_free1;

  if(mdp1->heapinfo[first_block_free].free.size != mdp2->heapinfo[first_block_free].free.size){ 
    if(XBT_LOG_ISENABLED(xbt_mm_legacy, xbt_log_priority_debug)){
      XBT_DEBUG("Different size (in blocks) of the first free cluster");
      errors++;
    }else{
      return 1;
    }
  }

  /* Check busy blocks (circular checking)*/

  i = first_block_free + mdp1->heapinfo[first_block_free].free.size;

  if(mdp1->heapinfo[first_block_free].free.next != mdp2->heapinfo[first_block_free].free.next){
    if(XBT_LOG_ISENABLED(xbt_mm_legacy, xbt_log_priority_debug)){
      XBT_DEBUG("Different next block free");
      errors++;
    }else{
      return 1;
    }
  }
  
  block_free = first_block_free;
  next_block_free = mdp1->heapinfo[first_block_free].free.next;

  if(next_block_free == 0)
    next_block_free = mdp1->heaplimit;

  while(i != first_block_free){

    while(i<next_block_free){

      if(mdp1->heapinfo[i].busy.type != mdp2->heapinfo[i].busy.type){
	if(XBT_LOG_ISENABLED(xbt_mm_legacy, xbt_log_priority_debug)){
	  XBT_DEBUG("Different type of busy block");
	  errors++;
	}else{
	  return 1;
	}
      }else{

	addr_block1 = (char *)mdp1 + sizeof(struct mdesc) + ((i-1) * BLOCKSIZE); 
	addr_block2 = (char *)mdp2 + sizeof(struct mdesc) + ((i-1) * BLOCKSIZE); 
	
	switch(mdp1->heapinfo[i].busy.type){
	case 0 :
	  if(mdp1->heapinfo[i].busy.info.block.size != mdp2->heapinfo[i].busy.info.block.size){
	    if(XBT_LOG_ISENABLED(xbt_mm_legacy, xbt_log_priority_debug)){
	      XBT_DEBUG("Different size of a large cluster");
	      errors++;
	    }else{
	      return 1;
	    }
	  }else{
	    if(memcmp(addr_block1, addr_block2, (mdp1->heapinfo[i].busy.info.block.size * BLOCKSIZE)) != 0){
	      if(XBT_LOG_ISENABLED(xbt_mm_legacy, xbt_log_priority_debug)){	      
		XBT_DEBUG("Different data in block %zu (size = %zu) (addr_block1 = %p (current = %p) - addr_block2 = %p)", i, mdp1->heapinfo[i].busy.info.block.size, addr_block1, (char *)std_heap_addr + sizeof(struct mdesc) + ((i-1) * BLOCKSIZE), addr_block2);
		errors++;
	      }else{
		return 1;
	      }
	    } 
	  }
	  i = i+mdp1->heapinfo[i].busy.info.block.size;

	  break;
	default :	  
	  if(mdp1->heapinfo[i].busy.info.frag.nfree != mdp2->heapinfo[i].busy.info.frag.nfree){
	    if(XBT_LOG_ISENABLED(xbt_mm_legacy, xbt_log_priority_debug)){
	      XBT_DEBUG("Different free fragments in the fragmented block %zu", i);
	      errors++;
	    }else{
	      return 1;
	    }
	  }else{
	    if(mdp1->heapinfo[i].busy.info.frag.first != mdp2->heapinfo[i].busy.info.frag.first){
	      if(XBT_LOG_ISENABLED(xbt_mm_legacy, xbt_log_priority_debug)){
		XBT_DEBUG("Different first free fragments in the block %zu", i);
		errors++;
	      }else{
		return 1;
	      } 
	    }else{
	      frag_size = pow(2,mdp1->heapinfo[i].busy.type);
	      for(j=0 ; j< (BLOCKSIZE/frag_size); j++){
		if(memcmp((char *)addr_block1 + (j * frag_size), (char *)addr_block2 + (j * frag_size), frag_size) != 0){
		  if(XBT_LOG_ISENABLED(xbt_mm_legacy, xbt_log_priority_debug)){
		    XBT_DEBUG("Different data in fragment %zu (addr_frag1 = %p - addr_frag2 = %p) of block %zu", j + 1, (char *)addr_block1 + (j * frag_size), (char *)addr_block2 + (j * frag_size),  i);
		    errors++;
		  }else{
		    return 1;
		  }
		} 
	      }
	    }
	  }

	  i++;

	  break;
	}

      }
    }

    if( i != first_block_free){

      if(mdp1->heapinfo[block_free].free.next != mdp2->heapinfo[block_free].free.next){
	if(XBT_LOG_ISENABLED(xbt_mm_legacy, xbt_log_priority_debug)){
	  XBT_DEBUG("Different next block free");
	  errors++;
	}else{
	  return 1;
	}
      }
     
      block_free = mdp1->heapinfo[block_free].free.next;
      next_block_free = mdp1->heapinfo[block_free].free.next;

      i = block_free + mdp1->heapinfo[block_free].free.size;

      if((next_block_free == 0) && (i != mdp1->heaplimit)){

	while(i < mdp1->heaplimit){

	  if(mdp1->heapinfo[i].busy.type != mdp2->heapinfo[i].busy.type){
	    if(XBT_LOG_ISENABLED(xbt_mm_legacy, xbt_log_priority_debug)){
	      XBT_DEBUG("Different type of busy block");
	      errors++;
	    }else{
	      return 1;
	    }
	  }else{

	    addr_block1 = (char *)mdp1 + sizeof(struct mdesc) + ((i-1) * BLOCKSIZE); 
	    addr_block2 = (char *)mdp2 + sizeof(struct mdesc) + ((i-1) * BLOCKSIZE); 
	
	    switch(mdp1->heapinfo[i].busy.type){
	    case 0 :
	      if(mdp1->heapinfo[i].busy.info.block.size != mdp2->heapinfo[i].busy.info.block.size){
		if(XBT_LOG_ISENABLED(xbt_mm_legacy, xbt_log_priority_debug)){
		  XBT_DEBUG("Different size of a large cluster");
		  errors++;
		}else{
		  return 1;
		}
	      }else{
		if(memcmp(addr_block1, addr_block2, (mdp1->heapinfo[i].busy.info.block.size * BLOCKSIZE)) != 0){
		  if(XBT_LOG_ISENABLED(xbt_mm_legacy, xbt_log_priority_debug)){	      
		    XBT_DEBUG("Different data in block %zu (addr_block1 = %p (current = %p) - addr_block2 = %p)", i, addr_block1, (char *)std_heap_addr + sizeof(struct mdesc) + ((i-1) * BLOCKSIZE), addr_block2);
		    errors++;
		  }else{
		    return 1;
		  }
		} 
	      }
	    
	      i = i+mdp1->heapinfo[i].busy.info.block.size;

	      break;
	    default :	  
	      if(mdp1->heapinfo[i].busy.info.frag.nfree != mdp2->heapinfo[i].busy.info.frag.nfree){
		if(XBT_LOG_ISENABLED(xbt_mm_legacy, xbt_log_priority_debug)){
		  XBT_DEBUG("Different free fragments in the fragmented block %zu", i);
		  errors++;
		}else{
		  return 1;
		}
	      }else{
		if(mdp1->heapinfo[i].busy.info.frag.first != mdp2->heapinfo[i].busy.info.frag.first){
		  if(XBT_LOG_ISENABLED(xbt_mm_legacy, xbt_log_priority_debug)){
		    XBT_DEBUG("Different first free fragments in the block %zu", i);
		    errors++;
		  }else{
		    return 1;
		  } 
		}else{
		  frag_size = pow(2,mdp1->heapinfo[i].busy.type);
		  for(j=0 ; j< (BLOCKSIZE/frag_size); j++){
		    if(memcmp((char *)addr_block1 + (j * frag_size), (char *)addr_block2 + (j * frag_size), frag_size) != 0){
		      if(XBT_LOG_ISENABLED(xbt_mm_legacy, xbt_log_priority_debug)){
			XBT_DEBUG("Different data in fragment %zu (addr_frag1 = %p - addr_frag2 = %p) of block %zu", j + 1, (char *)addr_block1 + (j * frag_size), (char *)addr_block2 + (j * frag_size),  i);
			errors++;
		      }else{
			return 1;
		      }
		    } 
		  }
		}
	      }

	      i++;

	      break;
	    }
	  }
	}

      }

    }
  }
  
  return (errors>0);    
}

 
void mmalloc_display_info_heap(void *h){

}  
  

