/* mm_diff - Memory snapshooting and comparison                             */

/* Copyright (c) 2008-2012. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex_interface.h" /* internals of backtrace setup */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mm_diff, xbt,
                                "Logging specific to mm_diff in mmalloc");

extern char *xbt_binary_name;

void mmalloc_backtrace_display(xbt_mheap_t mdp, void *ptr){
  size_t block = BLOCK(ptr);
  int type;
  xbt_ex_t e;

  if ((char *) ptr < (char *) mdp->heapbase || block > mdp->heapsize) {
    fprintf(stderr,"Ouch, this pointer is not mine. I cannot display its backtrace. I refuse it to death!!\n");
    abort();
  }

  type = mdp->heapinfo[block].type;

  if (type != 0) {
    //fprintf(stderr,"Only full blocks are backtraced for now. Ignoring your request.\n");
    return;
  }
  if (mdp->heapinfo[block].busy_block.bt_size == 0) {
    fprintf(stderr,"No backtrace available for that block, sorry.\n");
    return;
  }

  memcpy(&e.bt,&(mdp->heapinfo[block].busy_block.bt),sizeof(void*)*XBT_BACKTRACE_SIZE);
  e.used = mdp->heapinfo[block].busy_block.bt_size;

  xbt_ex_setup_backtrace(&e);
  if (e.used == 0) {
    fprintf(stderr, "(backtrace not set)\n");
  } else if (e.bt_strings == NULL) {
    fprintf(stderr, "(backtrace not ready to be computed. %s)\n",xbt_binary_name?"Dunno why":"xbt_binary_name not setup yet");
  } else {
    int i;

    fprintf(stderr, "Backtrace of where the block %p where malloced (%d frames):\n",ptr,e.used);
    for (i = 0; i < e.used; i++)       /* no need to display "xbt_backtrace_display" */{
      fprintf(stderr,"%d",i);fflush(NULL);
      fprintf(stderr, "---> %s\n", e.bt_strings[i] + 4);
    }
  }
}


void mmalloc_backtrace_block_display(xbt_mheap_t mdp, size_t block){

  int type;
  xbt_ex_t e;

  type = mdp->heapinfo[block].type;

  if (type != 0) {
    fprintf(stderr,"Only full blocks are backtraced for now. Ignoring your request.\n");
    return;
  }
  if (mdp->heapinfo[block].busy_block.bt_size == 0) {
    fprintf(stderr,"No backtrace available for that block, sorry.\n");
    return;
  }

  memcpy(&e.bt,&(mdp->heapinfo[block].busy_block.bt),sizeof(void*)*XBT_BACKTRACE_SIZE);
  e.used = mdp->heapinfo[block].busy_block.bt_size;

  xbt_ex_setup_backtrace(&e);
  if (e.used == 0) {
    fprintf(stderr, "(backtrace not set)\n");
  } else if (e.bt_strings == NULL) {
    fprintf(stderr, "(backtrace not ready to be computed. %s)\n",xbt_binary_name?"Dunno why":"xbt_binary_name not setup yet");
  } else {
    int i;

    fprintf(stderr, "Backtrace of where the block %zu where malloced (%d frames):\n", block ,e.used);
    for (i = 0; i < e.used; i++)       /* no need to display "xbt_backtrace_display" */{
      fprintf(stderr,"%d",i);fflush(NULL);
      fprintf(stderr, "---> %s\n", e.bt_strings[i] + 4);
    }
  }
}

int mmalloc_compare_heap(xbt_mheap_t mdp1, xbt_mheap_t mdp2){

  if(mdp1 == NULL && mdp2 == NULL){
    XBT_DEBUG("Malloc descriptors null\n");
    return 0;
  }

  int errors = mmalloc_compare_mdesc(mdp1, mdp2);

  return (errors > 0);

}

int mmalloc_compare_mdesc(struct mdesc *mdp1, struct mdesc *mdp2){

  int errors = 0;

  if(mdp1->headersize != mdp2->headersize){
    fprintf(stderr, "Different size of the file header for the mapped files\n");
    return 1;
  }

  if(mdp1->refcount != mdp2->refcount){
    fprintf(stderr, "Different number of processes that attached the heap\n");
    return 1;
  }

  if(strcmp(mdp1->magic, mdp2->magic) != 0){
    fprintf(stderr,"Different magic number\n");
    return 1;
  }

  if(mdp1->flags != mdp2->flags){
    fprintf(stderr,"Different flags\n");  
    return 1;
  }

  if(mdp1->heapsize != mdp2->heapsize){
    fprintf(stderr,"Different number of info entries\n");
    return 1;
  }
  

  if(mdp1->heapbase != mdp2->heapbase){
    fprintf(stderr,"Different first block of the heap\n");
    return 1;
  }


  if(mdp1->heapindex != mdp2->heapindex){
    fprintf(stderr,"Different index for the heap table : %zu - %zu\n", mdp1->heapindex, mdp2->heapindex);
    return 1;
  }


  if(mdp1->base != mdp2->base){
    fprintf(stderr,"Different base address of the memory region\n");
    return 1;
  }

  if(mdp1->breakval != mdp2->breakval){
    fprintf(stderr,"Different current location in the memory region\n");
    return 1;
  }

  if(mdp1->top != mdp2->top){
    fprintf(stderr,"Different end of the current location in the memory region\n");
    return 1;
  }

  if(mdp1->heaplimit != mdp2->heaplimit){
    fprintf(stderr,"Different limit of valid info table indices\n");
    return 1;
  }

  if(mdp1->fd != mdp2->fd){
    fprintf(stderr,"Different file descriptor for the file to which this malloc heap is mapped\n");
    return 1;
  }

  if(mdp1->version != mdp2->version){
    fprintf(stderr,"Different version of the mmalloc package\n");
    return 1;
  }


  size_t i, j;
  void *addr_block1, *addr_block2, *addr_frag1, *addr_frag2;
  size_t frag_size;

  i = 0;

  /* Check busy blocks*/

  while(i < mdp1->heapindex){

    if(mdp1->heapinfo[i].type != mdp2->heapinfo[i].type){
      fprintf(stderr,"Different type of block : %d - %d\n", mdp1->heapinfo[i].type, mdp2->heapinfo[i].type);
      errors++;
    }

    addr_block1 = (char *)mdp1 + sizeof(struct mdesc) + (i * BLOCKSIZE);
    addr_block2 = (char *)mdp2 + sizeof(struct mdesc) + (i * BLOCKSIZE);

    if(mdp1->heapinfo[i].type == 0){ /* busy large block */

      if(mdp1->heapinfo[i].busy_block.size != mdp2->heapinfo[i].busy_block.size){
	fprintf(stderr,"Different size of a large cluster : %zu - %zu\n", mdp1->heapinfo[i].busy_block.size, mdp2->heapinfo[i].busy_block.size);
	errors++;
      } 

      if(mdp1->heapinfo[i].busy_block.busy_size != mdp2->heapinfo[i].busy_block.busy_size){
	fprintf(stderr,"Different busy_size of a large cluster : %zu - %zu\n", mdp1->heapinfo[i].busy_block.busy_size, mdp2->heapinfo[i].busy_block.busy_size);
	errors++;
      } 

      if(memcmp(addr_block1, addr_block2, (mdp1->heapinfo[i].busy_block.busy_size)) != 0){
	fprintf(stderr,"Different data in large block %zu (size = %zu (in blocks), busy_size = %zu (in bytes))\n", i, mdp1->heapinfo[i].busy_block.size, mdp1->heapinfo[i].busy_block.busy_size);
	//fprintf(stderr, "Backtrace size : %d\n", mdp1->heapinfo[i].busy_block.bt_size);
	mmalloc_backtrace_block_display(mdp1, i);
	errors++;
      }

      //fprintf(stderr, "Backtrace size : %d\n", mdp1->heapinfo[i].busy_block.bt_size);
      //mmalloc_backtrace_block_display(mdp1, i);
	
      i = i + mdp1->heapinfo[i].busy_block.size;

    }else{
      
      if(mdp1->heapinfo[i].type > 0){ /* busy fragmented block */

	if(mdp1->heapinfo[i].type != mdp2->heapinfo[i].type){
	  fprintf(stderr,"Different size of fragments in fragmented block %zu : %d - %d\n", i, mdp1->heapinfo[i].type, mdp2->heapinfo[i].type);
	  errors++;
	}

	if(mdp1->heapinfo[i].busy_frag.nfree != mdp2->heapinfo[i].busy_frag.nfree){
	  fprintf(stderr,"Different free fragments in fragmented block %zu : %zu - %zu\n", i, mdp1->heapinfo[i].busy_frag.nfree, mdp2->heapinfo[i].busy_frag.nfree);
	  errors++;
	} 
	
	if(mdp1->heapinfo[i].busy_frag.first != mdp2->heapinfo[i].busy_frag.first){
	  fprintf(stderr,"Different busy_size of a large cluster : %zu - %zu\n", mdp1->heapinfo[i].busy_block.busy_size, mdp2->heapinfo[i].busy_block.busy_size);
	  errors++;
	} 

	frag_size = pow(2, mdp1->heapinfo[i].type);

	for(j=0; j< (BLOCKSIZE/frag_size); j++){

	  if(mdp1->heapinfo[i].busy_frag.frag_size[j] != mdp2->heapinfo[i].busy_frag.frag_size[j]){
	    fprintf(stderr,"Different busy_size for fragment %zu in block %zu : %hu - %hu\n", j, i, mdp1->heapinfo[i].busy_frag.frag_size[j], mdp2->heapinfo[i].busy_frag.frag_size[j]);
	    errors++;
	  }

	  if(mdp1->heapinfo[i].busy_frag.frag_size[j] > 0){
	    
	    addr_frag1 = (char *)addr_block1 + (j * frag_size);
	    addr_frag2 = (char *)addr_block2 + (j * frag_size);

	    if(memcmp(addr_frag1, addr_frag2, mdp1->heapinfo[i].busy_frag.frag_size[j]) != 0){
	      fprintf(stderr,"Different data in fragment %zu (size = %zu, size used = %hu) in block %zu \n", j, frag_size, mdp1->heapinfo[i].busy_frag.frag_size[j], i);
	      errors++;
	    }

	  }
	}

	i++;

      }else{ /* free block */

	i++;

      }
      
    }

  }


  return (errors);
}


void mmalloc_display_info_heap(xbt_mheap_t h){

}



