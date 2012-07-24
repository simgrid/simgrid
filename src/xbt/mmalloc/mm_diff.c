/* mm_diff - Memory snapshooting and comparison                             */

/* Copyright (c) 2008-2012. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex_interface.h" /* internals of backtrace setup */
#include "xbt/str.h"
#include "mc/mc.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mm_diff, xbt,
                                "Logging specific to mm_diff in mmalloc");

extern char *xbt_binary_name;

static int compare_area(void *area1, void* area2, size_t size, xbt_dynar_t previous);
static void heap_area_pair_free(heap_area_pair_t pair);
static void heap_area_pair_free_voidp(void *d);
static int add_heap_area_pair(xbt_dynar_t list, int block1, int fragment1, int block2, int fragment2);
static int is_new_heap_area_pair(xbt_dynar_t list, int block1, int fragment1, int block2, int fragment2);
static void match_equals(xbt_dynar_t list);


void mmalloc_backtrace_block_display(void* heapinfo, size_t block){

  xbt_ex_t e;

  if (((malloc_info *)heapinfo)[block].busy_block.bt_size == 0) {
    fprintf(stderr,"No backtrace available for that block, sorry.\n");
    return;
  }

  memcpy(&e.bt,&(((malloc_info *)heapinfo)[block].busy_block.bt),sizeof(void*)*XBT_BACKTRACE_SIZE);
e.used = ((malloc_info *)heapinfo)[block].busy_block.bt_size;

  xbt_ex_setup_backtrace(&e);
  if (e.used == 0) {
    fprintf(stderr, "(backtrace not set)\n");
  } else if (e.bt_strings == NULL) {
    fprintf(stderr, "(backtrace not ready to be computed. %s)\n",xbt_binary_name?"Dunno why":"xbt_binary_name not setup yet");
  } else {
    int i;

    fprintf(stderr, "Backtrace of where the block %zu was malloced (%d frames):\n", block ,e.used);
    for (i = 0; i < e.used; i++)       /* no need to display "xbt_backtrace_display" */{
      fprintf(stderr,"%d",i);fflush(NULL);
      fprintf(stderr, "---> %s\n", e.bt_strings[i] + 4);
    }
  }
}

void mmalloc_backtrace_fragment_display(void* heapinfo, size_t block, size_t frag){

  xbt_ex_t e;

  memcpy(&e.bt,&(((malloc_info *)heapinfo)[block].busy_frag.bt[frag]),sizeof(void*)*XBT_BACKTRACE_SIZE);
  e.used = XBT_BACKTRACE_SIZE;

  xbt_ex_setup_backtrace(&e);
  if (e.used == 0) {
    fprintf(stderr, "(backtrace not set)\n");
  } else if (e.bt_strings == NULL) {
    fprintf(stderr, "(backtrace not ready to be computed. %s)\n",xbt_binary_name?"Dunno why":"xbt_binary_name not setup yet");
  } else {
    int i;

    fprintf(stderr, "Backtrace of where the fragment %zu in block %zu was malloced (%d frames):\n", frag, block ,e.used);
    for (i = 0; i < e.used; i++)       /* no need to display "xbt_backtrace_display" */{
      fprintf(stderr,"%d",i);fflush(NULL);
      fprintf(stderr, "---> %s\n", e.bt_strings[i] + 4);
    }
  }
}


malloc_info *heapinfo1 =  NULL, *heapinfo2 = NULL;
size_t heapsize1, heapsize2, heaplimit;
void *s_heap = NULL, *heapbase1 = NULL, *heapbase2 = NULL;

int mmalloc_compare_heap(xbt_mheap_t mdp1, xbt_mheap_t mdp2){

  if(mdp1 == NULL && mdp2 == NULL){
    fprintf(stderr, "Malloc descriptors null\n");
    return 0;
  }

  if(mdp1->heaplimit != mdp2->heaplimit){
    fprintf(stderr,"Different limit of valid info table indices\n");
    return 1;
  }

  /* Heap information */
  heaplimit = mdp1->heaplimit;

  s_heap = (char *)mmalloc_get_current_heap() - STD_HEAP_SIZE - getpagesize();

  heapbase1 = (char *)mdp1 + BLOCKSIZE;
  heapbase2 = (char *)mdp2 + BLOCKSIZE;

  heapinfo1 = (malloc_info *)((char *)mdp1 + ((char *)mdp1->heapinfo - (char *)s_heap));
  heapinfo2 = (malloc_info *)((char *)mdp2 + ((char *)mdp2->heapinfo - (char *)s_heap));

  heapsize1 = mdp1->heapsize;
  heapsize2 = mdp2->heapsize;


  /* Start comparison */
  size_t i1 = 1, i2, j1, j2;
  void *addr_block1 = NULL, *addr_block2 = NULL, *addr_frag1 = NULL, *addr_frag2 = NULL;
  size_t frag_size1, frag_size2;

  xbt_dynar_t previous = xbt_dynar_new(sizeof(heap_area_pair_t), heap_area_pair_free_voidp);

  int equal = 0, k;
  
  /* Check busy blocks*/

  while(i1 <= heaplimit){

    i2 = 1;
    equal = 0;

    if(heapinfo1[i1].type == -1){ /* Free block */
      i1++;
      continue;
    }

    addr_block1 = ((void*) (((ADDR2UINT(i1)) - 1) * BLOCKSIZE + (char*)heapbase1));

    if(heapinfo1[i1].type == 0){  /* Large block */

      while(i2 <= heaplimit && !equal){

        if(heapinfo2[i2].type != 0){
          i2++;
          continue;
        }

        if(heapinfo2[i2].busy_block.equal_to == 1){         
          i2++;
          continue;
        }
        
        if(heapinfo1[i1].busy_block.size != heapinfo2[i2].busy_block.size){
          i2++;
          continue;
        }
        
        if(heapinfo1[i1].busy_block.busy_size != heapinfo2[i2].busy_block.busy_size){
          i2++;
          continue;
        }

        addr_block2 = ((void*) (((ADDR2UINT(i2)) - 1) * BLOCKSIZE + (char*)heapbase2));
        
        /* Comparison */
        add_heap_area_pair(previous, i1, -1, i2, -1);
        if(!compare_area(addr_block1, addr_block2, heapinfo1[i1].busy_block.busy_size, previous)){
          for(k=0; k < heapinfo2[i2].busy_block.size; k++)
            heapinfo2[i2+k].busy_block.equal_to = 1;
          for(k=0; k < heapinfo1[i1].busy_block.size; k++)
            heapinfo1[i1+k].busy_block.equal_to = 1;
          equal = 1;
          match_equals(previous);
        }
        xbt_dynar_reset(previous);

        i2++;

      }

    }else{ /* Fragmented block */

      frag_size1 = pow(2, heapinfo1[i1].type);

      for(j1=0; j1<(BLOCKSIZE/frag_size1); j1++){

        if(heapinfo1[i1].busy_frag.frag_size[j1] == 0) /* Free fragment */
          continue;
        
        addr_frag1 = (char *)addr_block1 + (j1 * frag_size1);
        
        i2 = 1;
        equal = 0;
        
        while(i2 <= heaplimit && !equal){
          
          if(heapinfo2[i2].type <= 0){
            i2++;
            continue;
          }
          
          frag_size2 = pow(2, heapinfo2[i2].type);

          for(j2=0; j2< (BLOCKSIZE/frag_size2); j2++){

            if(heapinfo2[i2].busy_frag.equal_to[j2] == 1){                 
              continue;              
            }
             
            if(heapinfo1[i1].busy_frag.frag_size[j1] != heapinfo2[i2].busy_frag.frag_size[j2]){ /* Different size_used */    
              continue;
            }
             
            addr_block2 = ((void*) (((ADDR2UINT(i2)) - 1) * BLOCKSIZE + (char*)heapbase2));
            addr_frag2 = (char *)addr_block2 + (j2 * frag_size2);
             
            /* Comparison */
            add_heap_area_pair(previous, i1, j1, i2, j2);
            if(!compare_area(addr_frag1, addr_frag2, heapinfo1[i1].busy_frag.frag_size[j1], previous)){
              heapinfo2[i2].busy_frag.equal_to[j2] = 1;
              heapinfo1[i1].busy_frag.equal_to[j1] = 1;
              equal = 1;
              match_equals(previous);
              break;
            }
            xbt_dynar_reset(previous);

          }

          i2++;

        }

      }

    }

    i1++;

  }

  /* All blocks/fragments are equal to another block/fragment ? */
  int i = 1, j;
  int nb_diff1 = 0, nb_diff2 = 0;
  int frag_size;
 
  while(i<=heaplimit){
    if(heapinfo1[i].type == 0){
      if(heapinfo1[i].busy_block.busy_size > 0){
        if(heapinfo1[i].busy_block.equal_to == -1){
          addr_block1 = ((void*) (((ADDR2UINT((size_t)i)) - 1) * BLOCKSIZE + (char*)heapbase1));
          fprintf(stderr, "Block %d (%p) not found (size used = %zu)\n", i, addr_block1, heapinfo1[i].busy_block.busy_size);
          mmalloc_backtrace_block_display(heapinfo1, i);
          nb_diff1++;
        }
      }
    }
    if(heapinfo1[i].type > 0){
      frag_size = pow(2, heapinfo1[i].type);
      for(j=0; j < BLOCKSIZE/frag_size; j++){
        if(heapinfo1[i].busy_frag.frag_size[j] > 0){
          if(heapinfo1[i].busy_frag.equal_to[j] == -1){
            addr_block1 = ((void*) (((ADDR2UINT((size_t)i)) - 1) * BLOCKSIZE + (char*)heapbase1));
            addr_frag1 = (char *)addr_block1 + (j * frag_size);
            fprintf(stderr, "Block %d, Fragment %d (%p) not found (size used = %d)\n", i, j, addr_frag1, heapinfo1[i].busy_frag.frag_size[j]);
            mmalloc_backtrace_fragment_display(heapinfo1, i, j);
            nb_diff1++;
          }
        }
      }
    }
    
    i++; 
  }

  fprintf(stderr, "Different blocks or fragments in heap1 : %d\n", nb_diff1);

  i = 1;

  while(i<=heaplimit){
    if(heapinfo2[i].type == 0){
      if(heapinfo2[i].busy_block.busy_size > 0){
        if(heapinfo2[i].busy_block.equal_to == -1){
          addr_block2 = ((void*) (((ADDR2UINT((size_t)i)) - 1) * BLOCKSIZE + (char*)heapbase2));
          fprintf(stderr, "Block %d (%p) not found (size used = %zu)\n", i, addr_block2, heapinfo2[i].busy_block.busy_size);
          mmalloc_backtrace_block_display(heapinfo2, i);
          nb_diff2++;
        }
      }
    }
    if(heapinfo2[i].type > 0){
      frag_size = pow(2, heapinfo2[i].type);
      for(j=0; j < BLOCKSIZE/frag_size; j++){
        if(heapinfo2[i].busy_frag.frag_size[j] > 0){
          if(heapinfo2[i].busy_frag.equal_to[j] == -1){
            addr_block2 = ((void*) (((ADDR2UINT((size_t)i)) - 1) * BLOCKSIZE + (char*)heapbase2));
            addr_frag2 = (char *)addr_block2 + (j * frag_size);
            fprintf(stderr, "Block %d, Fragment %d (%p) not found (size used = %d)\n", i, j, addr_frag2, heapinfo2[i].busy_frag.frag_size[j]);
            mmalloc_backtrace_fragment_display(heapinfo2, i, j);
            nb_diff2++;
          }
        }
      }
    }
    i++; 
  }

  fprintf(stderr, "Different blocks or fragments in heap2 : %d\n", nb_diff2);
  
  
  /* Reset equal information */
  i = 1;

  while(i<=heaplimit){
    if(heapinfo1[i].type == 0){
      heapinfo1[i].busy_block.equal_to = -1;
    }
    if(heapinfo1[i].type > 0){
      for(j=0; j < MAX_FRAGMENT_PER_BLOCK; j++){
        heapinfo1[i].busy_frag.equal_to[j] = -1;
      }
    }
    i++; 
  }

  i = 1;

  while(i<=heaplimit){
    if(heapinfo2[i].type == 0){
      heapinfo2[i].busy_block.equal_to = -1;
    }
    if(heapinfo2[i].type > 0){
      for(j=0; j < MAX_FRAGMENT_PER_BLOCK; j++){
        heapinfo2[i].busy_frag.equal_to[j] = -1;
      }
    }
    i++; 
  }

  xbt_dynar_free(&previous);
  addr_block1 = NULL, addr_block2 = NULL, addr_frag1 = NULL, addr_frag2 = NULL;
  s_heap = NULL, heapbase1 = NULL, heapbase2 = NULL;

  return (nb_diff1 > 0 || nb_diff2 > 0);

}


static int compare_area(void *area1, void* area2, size_t size, xbt_dynar_t previous){

  int i = 0, pointer_align;
  void *address_pointed1 = NULL, *address_pointed2 = NULL, *addr_block1 = NULL, *addr_block2 = NULL, *addr_frag1 = NULL, *addr_frag2 = NULL;
  int block_pointed1, block_pointed2, frag_pointed1, frag_pointed2;
  int frag_size;
  int res_compare;
  
  while(i<size){

    if(memcmp(((char *)area1) + i, ((char *)area2) + i, 1) != 0){

      /* Check pointer difference */
      pointer_align = (i / sizeof(void*)) * sizeof(void*);
      address_pointed1 = *((void **)((char *)area1 + pointer_align));
      address_pointed2 = *((void **)((char *)area2 + pointer_align));

      /* Get pointed blocks number */ 
      block_pointed1 = ((char*)address_pointed1 - (char*)((struct mdesc*)s_heap)->heapbase) / BLOCKSIZE + 1;
      block_pointed2 = ((char*)address_pointed2 - (char*)((struct mdesc*)s_heap)->heapbase) / BLOCKSIZE + 1;

      /* Check if valid blocks number */
      if((char *)address_pointed1 < (char*)((struct mdesc*)s_heap)->heapbase || block_pointed1 > heapsize1 || block_pointed1 < 1 || (char *)address_pointed2 < (char*)((struct mdesc*)s_heap)->heapbase || block_pointed2 > heapsize2 || block_pointed2 < 1)
        return 1;

      if(heapinfo1[block_pointed1].type == heapinfo2[block_pointed2].type){ /* Same type of block (large or fragmented) */

        addr_block1 = ((void*) (((ADDR2UINT((size_t)block_pointed1)) - 1) * BLOCKSIZE + (char*)heapbase1));
        addr_block2 = ((void*) (((ADDR2UINT((size_t)block_pointed2)) - 1) * BLOCKSIZE + (char*)heapbase2));
        
        if(heapinfo1[block_pointed1].type == 0){ /* Large block */

          if(heapinfo1[block_pointed1].busy_block.size != heapinfo2[block_pointed2].busy_block.size){
            return 1;
          }

          if(heapinfo1[block_pointed1].busy_block.busy_size != heapinfo2[block_pointed2].busy_block.busy_size){
            return 1;
          }

          if(add_heap_area_pair(previous, block_pointed1, -1, block_pointed2, -1)){
            
            res_compare = compare_area(addr_block1, addr_block2, heapinfo1[block_pointed1].busy_block.busy_size, previous);
            
            if(res_compare)    
              return 1;
           
          }
          
        }else{ /* Fragmented block */

           /* Get pointed fragments number */ 
          frag_pointed1 = ((uintptr_t) (ADDR2UINT (address_pointed1) % (BLOCKSIZE))) >> heapinfo1[block_pointed1].type;
          frag_pointed2 = ((uintptr_t) (ADDR2UINT (address_pointed2) % (BLOCKSIZE))) >> heapinfo2[block_pointed2].type;
         
          if(heapinfo1[block_pointed1].busy_frag.frag_size[frag_pointed1] != heapinfo2[block_pointed2].busy_frag.frag_size[frag_pointed2]) /* Different size_used */    
            return 1;

          frag_size = pow(2, heapinfo1[block_pointed1].type);
            
          addr_frag1 = (char *)addr_block1 + (frag_pointed1 * frag_size);
          addr_frag2 = (char *)addr_block2 + (frag_pointed2 * frag_size);

          if(add_heap_area_pair(previous, block_pointed1, frag_pointed1, block_pointed2, frag_pointed2)){

            res_compare = compare_area(addr_frag1, addr_frag2, heapinfo1[block_pointed1].busy_frag.frag_size[frag_pointed1], previous);
            
            if(res_compare)
              return 1;
           
          }
          
        }
          
      }

      i = pointer_align + sizeof(void *);
      
    }else{

      i++;

    }
  }

  address_pointed1 = NULL, address_pointed2 = NULL, addr_block1 = NULL, addr_block2 = NULL, addr_frag1 = NULL, addr_frag2 = NULL;

  return 0;
  

}

static void heap_area_pair_free(heap_area_pair_t pair){
  if (pair){
    free(pair);
    pair = NULL;
  }
}

static void heap_area_pair_free_voidp(void *d)
{
  heap_area_pair_free((heap_area_pair_t) * (void **) d);
}

static int add_heap_area_pair(xbt_dynar_t list, int block1, int fragment1, int block2, int fragment2){

  if(is_new_heap_area_pair(list, block1, fragment1, block2, fragment2)){
    heap_area_pair_t pair = NULL;
    pair = xbt_new0(s_heap_area_pair_t, 1);
    pair->block1 = block1;
    pair->fragment1 = fragment1;
    pair->block2 = block2;
    pair->fragment2 = fragment2;
    
    xbt_dynar_push(list, &pair); 

    return 1;
  }

  return 0;
}
 
static int is_new_heap_area_pair(xbt_dynar_t list, int block1, int fragment1, int block2, int fragment2){
  
  unsigned int cursor = 0;
  heap_area_pair_t current_pair = NULL;

  xbt_dynar_foreach(list, cursor, current_pair){
    if(current_pair->block1 == block1 && current_pair->block2 == block2 && current_pair->fragment1 == fragment1 && current_pair->fragment2 == fragment2)
      return 0; 
  }

  current_pair = NULL;
  
  return 1;
}

static void match_equals(xbt_dynar_t list){

  unsigned int cursor = 0;
  heap_area_pair_t current_pair = NULL;

  xbt_dynar_foreach(list, cursor, current_pair){
    if(current_pair->fragment1 != -1){
      heapinfo1[current_pair->block1].busy_frag.equal_to[current_pair->fragment1] = 1;
      heapinfo2[current_pair->block2].busy_frag.equal_to[current_pair->fragment2] = 1;
    }else{
      heapinfo1[current_pair->block1].busy_block.equal_to = 1;
      heapinfo2[current_pair->block2].busy_block.equal_to = 1;
    }
  }

  current_pair = NULL;

}

