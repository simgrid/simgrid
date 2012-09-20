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

xbt_dynar_t mmalloc_ignore;

typedef struct s_heap_area_pair{
  int block1;
  int fragment1;
  int block2;
  int fragment2;
}s_heap_area_pair_t, *heap_area_pair_t;

static void heap_area_pair_free(heap_area_pair_t pair);
static void heap_area_pair_free_voidp(void *d);
static int add_heap_area_pair(xbt_dynar_t list, int block1, int fragment1, int block2, int fragment2);
static int is_new_heap_area_pair(xbt_dynar_t list, int block1, int fragment1, int block2, int fragment2);

static int compare_area(void *area1, void* area2, size_t size, xbt_dynar_t previous, int check_ignore);
static void match_equals(xbt_dynar_t list);

static int in_mmalloc_ignore(int block, int fragment);
static size_t heap_comparison_ignore(void *address);

void mmalloc_backtrace_block_display(void* heapinfo, int block){

  xbt_ex_t e;

  if (((malloc_info *)heapinfo)[block].busy_block.bt_size == 0) {
    XBT_DEBUG("No backtrace available for that block, sorry.");
    return;
  }

  memcpy(&e.bt,&(((malloc_info *)heapinfo)[block].busy_block.bt),sizeof(void*)*XBT_BACKTRACE_SIZE);
  e.used = ((malloc_info *)heapinfo)[block].busy_block.bt_size;

  xbt_ex_setup_backtrace(&e);
  if (e.used == 0) {
    XBT_DEBUG("(backtrace not set)");
  } else if (e.bt_strings == NULL) {
    XBT_DEBUG("(backtrace not ready to be computed. %s)",xbt_binary_name?"Dunno why":"xbt_binary_name not setup yet");
  } else {
    int i;

    XBT_DEBUG("Backtrace of where the block %d was malloced (%d frames):", block ,e.used);
    for (i = 0; i < e.used; i++)       /* no need to display "xbt_backtrace_display" */{
      XBT_DEBUG("%d ---> %s",i, e.bt_strings[i] + 4);
    }
  }

}

void mmalloc_backtrace_fragment_display(void* heapinfo, int block, int frag){

  xbt_ex_t e;

  memcpy(&e.bt,&(((malloc_info *)heapinfo)[block].busy_frag.bt[frag]),sizeof(void*)*XBT_BACKTRACE_SIZE);
  e.used = XBT_BACKTRACE_SIZE;

  xbt_ex_setup_backtrace(&e);
  if (e.used == 0) {
    XBT_DEBUG("(backtrace not set)");
  } else if (e.bt_strings == NULL) {
    XBT_DEBUG("(backtrace not ready to be computed. %s)",xbt_binary_name?"Dunno why":"xbt_binary_name not setup yet");
  } else {
    int i;

    XBT_DEBUG("Backtrace of where the fragment %d in block %d was malloced (%d frames):", frag, block ,e.used);
    for (i = 0; i < e.used; i++)       /* no need to display "xbt_backtrace_display" */{
      XBT_DEBUG("%d ---> %s",i, e.bt_strings[i] + 4);
    }
  }

}

void *s_heap, *heapbase1, *heapbase2;
malloc_info *heapinfo1, *heapinfo2;
size_t heaplimit, heapsize1, heapsize2;

int ignore_done;

int mmalloc_compare_heap(xbt_mheap_t heap1, xbt_mheap_t heap2){

  if(heap1 == NULL && heap1 == NULL){
    XBT_DEBUG("Malloc descriptors null");
    return 0;
  }

  if(heap1->heaplimit != heap2->heaplimit){
    XBT_DEBUG("Different limit of valid info table indices");
    return 1;
  }

  /* Heap information */
  heaplimit = ((struct mdesc *)heap1)->heaplimit;

  s_heap = (char *)mmalloc_get_current_heap() - STD_HEAP_SIZE - getpagesize();

  heapbase1 = (char *)heap1 + BLOCKSIZE;
  heapbase2 = (char *)heap2 + BLOCKSIZE;

  heapinfo1 = (malloc_info *)((char *)heap1 + ((uintptr_t)((char *)heap1->heapinfo - (char *)s_heap)));
  heapinfo2 = (malloc_info *)((char *)heap2 + ((uintptr_t)((char *)heap2->heapinfo - (char *)s_heap)));

  heapsize1 = heap1->heapsize;
  heapsize2 = heap2->heapsize;

  /* Start comparison */
  size_t i1, i2, j1, j2, k, current_block, current_fragment;
  void *addr_block1, *addr_block2, *addr_frag1, *addr_frag2;

  xbt_dynar_t previous = xbt_dynar_new(sizeof(heap_area_pair_t), heap_area_pair_free_voidp);

  int equal, res_compare;

  ignore_done = 0;

  /* Init equal information */
  i1 = 1;

  while(i1<=heaplimit){
    if(heapinfo1[i1].type == 0){
      heapinfo1[i1].busy_block.equal_to = -1;
    }
    if(heapinfo1[i1].type > 0){
      for(j1=0; j1 < MAX_FRAGMENT_PER_BLOCK; j1++){
        heapinfo1[i1].busy_frag.equal_to[j1] = -1;
      }
    }
    i1++; 
  }

  i2 = 1;

  while(i2<=heaplimit){
    if(heapinfo2[i2].type == 0){
      heapinfo2[i2].busy_block.equal_to = -1;
    }
    if(heapinfo2[i2].type > 0){
      for(j2=0; j2 < MAX_FRAGMENT_PER_BLOCK; j2++){
        heapinfo2[i2].busy_frag.equal_to[j2] = -1;
      }
    }
    i2++; 
  }

  /* Check busy blocks*/

  i1 = 1;

  while(i1 <= heaplimit){

    current_block = i1;

    if(heapinfo1[i1].type == -1){ /* Free block */
      i1++;
      continue;
    }

    addr_block1 = ((void*) (((ADDR2UINT(i1)) - 1) * BLOCKSIZE + (char*)heapbase1));

    if(heapinfo1[i1].type == 0){  /* Large block */

      if(heapinfo1[i1].busy_block.busy_size == 0){
        i1++;
        continue;
      }
      
      i2 = 1;
      equal = 0;
      
      /* Try first to associate to same block in the other heap */
      if(heapinfo2[current_block].type == heapinfo1[current_block].type){
        
        if(heapinfo1[current_block].busy_block.busy_size == heapinfo2[current_block].busy_block.busy_size){

          addr_block2 = ((void*) (((ADDR2UINT(current_block)) - 1) * BLOCKSIZE + (char*)heapbase2));
          
          add_heap_area_pair(previous, current_block, -1, current_block, -1);
          
          if(ignore_done < xbt_dynar_length(mmalloc_ignore)){
            if(in_mmalloc_ignore((int)current_block, -1))
              res_compare = compare_area(addr_block1, addr_block2, heapinfo1[current_block].busy_block.busy_size, previous, 1);
            else
              res_compare = compare_area(addr_block1, addr_block2, heapinfo1[current_block].busy_block.busy_size, previous, 0);
          }else{
            res_compare = compare_area(addr_block1, addr_block2, heapinfo1[current_block].busy_block.busy_size, previous, 0);
          }
          
          if(res_compare == 0){
            for(k=1; k < heapinfo2[current_block].busy_block.size; k++)
              heapinfo2[current_block+k].busy_block.equal_to = 1 ;
            for(k=1; k < heapinfo1[current_block].busy_block.size; k++)
              heapinfo1[current_block+k].busy_block.equal_to = 1 ;
            equal = 1;
            match_equals(previous);
            i1 = i1 + heapinfo1[i1].busy_block.size;
          }

          xbt_dynar_reset(previous);
          
        }

      }

      while(i2 <= heaplimit && !equal){

        if(i2 == current_block){
          i2++;
          continue;
        }

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
        
        if(ignore_done < xbt_dynar_length(mmalloc_ignore)){
          if(in_mmalloc_ignore((int)i1, -1))
            res_compare = compare_area(addr_block1, addr_block2, heapinfo1[i1].busy_block.busy_size, previous, 1);
          else
            res_compare = compare_area(addr_block1, addr_block2, heapinfo1[i1].busy_block.busy_size, previous, 0);
        }else{
          res_compare = compare_area(addr_block1, addr_block2, heapinfo1[i1].busy_block.busy_size, previous, 0);
        }
        
        if(!res_compare){
          for(k=1; k < heapinfo2[i2].busy_block.size; k++)
            heapinfo2[i2+k].busy_block.equal_to = 1;
          for(k=1; k < heapinfo1[i1].busy_block.size; k++)
            heapinfo1[i1+k].busy_block.equal_to = 1;
          equal = 1;
          match_equals(previous);
        }

        xbt_dynar_reset(previous);

        i2++;

      }

      if(!equal)
        i1++;
      
    }else{ /* Fragmented block */

      for(j1=0; j1 < (size_t) (BLOCKSIZE >> heapinfo1[i1].type); j1++){

        current_fragment = j1;

        if(heapinfo1[i1].busy_frag.frag_size[j1] == 0) /* Free fragment */
          continue;
        
        addr_frag1 = (void*) ((char *)addr_block1 + (j1 << heapinfo1[i1].type));

        i2 = 1;
        equal = 0;
        
        /* Try first to associate to same fragment in the other heap */
        if(heapinfo2[current_block].type == heapinfo1[current_block].type){
          
          if(heapinfo1[current_block].busy_frag.frag_size[current_fragment] == heapinfo2[current_block].busy_frag.frag_size[current_fragment]){

            addr_block2 = ((void*) (((ADDR2UINT(current_block)) - 1) * BLOCKSIZE + (char*)heapbase2));
            addr_frag2 = (void*) ((char *)addr_block2 + (current_fragment << heapinfo2[current_block].type));

            add_heap_area_pair(previous, current_block, current_fragment, current_block, current_fragment);
            
            if(ignore_done < xbt_dynar_length(mmalloc_ignore)){
              if(in_mmalloc_ignore((int)current_block, (int)current_fragment))
                res_compare = compare_area(addr_frag1, addr_frag2, heapinfo1[current_block].busy_frag.frag_size[current_fragment], previous, 1);
              else
                res_compare = compare_area(addr_frag1, addr_frag2, heapinfo1[current_block].busy_frag.frag_size[current_fragment], previous, 0);
            }else{
              res_compare = compare_area(addr_frag1, addr_frag2, heapinfo1[current_block].busy_frag.frag_size[current_fragment], previous, 0);
            }

            if(res_compare == 0){
              equal = 1;
              match_equals(previous);
            }
            
            xbt_dynar_reset(previous);
            
          }

        }


        while(i2 <= heaplimit && !equal){
          
          if(heapinfo2[i2].type <= 0){
            i2++;
            continue;
          }

          for(j2=0; j2 < (size_t) (BLOCKSIZE >> heapinfo2[i2].type); j2++){

            if(heapinfo2[i2].type == heapinfo1[i1].type && i2 == current_block && j2 == current_fragment)
              continue;

            if(heapinfo2[i2].busy_frag.equal_to[j2] == 1)                
              continue;              
             
            if(heapinfo1[i1].busy_frag.frag_size[j1] != heapinfo2[i2].busy_frag.frag_size[j2]) /* Different size_used */    
              continue;
             
            addr_block2 = ((void*) (((ADDR2UINT(i2)) - 1) * BLOCKSIZE + (char*)heapbase2));
            addr_frag2 = (void*) ((char *)addr_block2 + (j2 << heapinfo2[i2].type));
             
            /* Comparison */
            add_heap_area_pair(previous, i1, j1, i2, j2);
            
            if(ignore_done < xbt_dynar_length(mmalloc_ignore)){
              if(in_mmalloc_ignore((int)i1, (int)j1))
                res_compare = compare_area(addr_frag1, addr_frag2, heapinfo1[i1].busy_frag.frag_size[j1], previous, 1);
              else
                res_compare = compare_area(addr_frag1, addr_frag2, heapinfo1[i1].busy_frag.frag_size[j1], previous, 0);
            }else{
              res_compare = compare_area(addr_frag1, addr_frag2, heapinfo1[i1].busy_frag.frag_size[j1], previous, 0);
            }

            if(!res_compare){
              equal = 1;
              match_equals(previous);
              xbt_dynar_reset(previous);
              break;
            }

            xbt_dynar_reset(previous);

          }

          i2++;

        }

      }

      i1++;
      
    }

  }

  /* All blocks/fragments are equal to another block/fragment ? */
  size_t i = 1, j = 0;
  int nb_diff1 = 0, nb_diff2 = 0;
 
  while(i<heaplimit){
    if(heapinfo1[i].type == 0){
      if(heapinfo1[i].busy_block.busy_size > 0){
        if(heapinfo1[i].busy_block.equal_to == -1){
          if(XBT_LOG_ISENABLED(mm_diff, xbt_log_priority_debug)){
            addr_block1 = ((void*) (((ADDR2UINT(i)) - 1) * BLOCKSIZE + (char*)heapbase1));
            XBT_DEBUG("Block %zu (%p) not found (size used = %zu)", i, addr_block1, heapinfo1[i].busy_block.busy_size);
            mmalloc_backtrace_block_display((void*)heapinfo1, i);
          }
          nb_diff1++;
        }
      }
    }
    if(heapinfo1[i].type > 0){
      for(j=0; j < (size_t) (BLOCKSIZE >> heapinfo1[i].type); j++){
        if(heapinfo1[i].busy_frag.frag_size[j] > 0){
          if(heapinfo1[i].busy_frag.equal_to[j] == -1){
            if(XBT_LOG_ISENABLED(mm_diff, xbt_log_priority_debug)){
              addr_block1 = ((void*) (((ADDR2UINT(i)) - 1) * BLOCKSIZE + (char*)heapbase1));
              addr_frag1 = (void*) ((char *)addr_block1 + (j << heapinfo1[i].type));
              XBT_DEBUG("Block %zu, Fragment %zu (%p) not found (size used = %d)", i, j, addr_frag1, heapinfo1[i].busy_frag.frag_size[j]);
              mmalloc_backtrace_fragment_display((void*)heapinfo1, i, j);
            }
            nb_diff1++;
          }
        }
      }
    }
    
    i++; 
  }

  XBT_DEBUG("Different blocks or fragments in heap1 : %d\n", nb_diff1);

  i = 1;

  while(i<heaplimit){
    if(heapinfo2[i].type == 0){
      if(heapinfo2[i].busy_block.busy_size > 0){
        if(heapinfo2[i].busy_block.equal_to == -1){
          if(XBT_LOG_ISENABLED(mm_diff, xbt_log_priority_debug)){
            addr_block2 = ((void*) (((ADDR2UINT(i)) - 1) * BLOCKSIZE + (char*)heapbase2));
            XBT_DEBUG("Block %zu (%p) not found (size used = %zu)", i, addr_block2, heapinfo2[i].busy_block.busy_size);
            mmalloc_backtrace_block_display((void*)heapinfo2, i);
          }
          nb_diff2++;
        }
      }
    }
    if(heapinfo2[i].type > 0){
      for(j=0; j < (size_t) (BLOCKSIZE >> heapinfo2[i].type); j++){
        if(heapinfo2[i].busy_frag.frag_size[j] > 0){
          if(heapinfo2[i].busy_frag.equal_to[j] == -1){
            if(XBT_LOG_ISENABLED(mm_diff, xbt_log_priority_debug)){
              addr_block2 = ((void*) (((ADDR2UINT(i)) - 1) * BLOCKSIZE + (char*)heapbase2));
              addr_frag2 = (void*) ((char *)addr_block2 + (j << heapinfo2[i].type));
              XBT_DEBUG( "Block %zu, Fragment %zu (%p) not found (size used = %d)", i, j, addr_frag2, heapinfo2[i].busy_frag.frag_size[j]);
              mmalloc_backtrace_fragment_display((void*)heapinfo2, i, j);
            }
            nb_diff2++;
          }
        }
      }
    }
    i++; 
  }

  XBT_DEBUG("Different blocks or fragments in heap2 : %d\n", nb_diff2);

  xbt_dynar_free(&previous);
 
  return ((nb_diff1 > 0) || (nb_diff2 > 0));

}

static int in_mmalloc_ignore(int block, int fragment){

  unsigned int cursor = 0;
  int start = 0;
  int end = xbt_dynar_length(mmalloc_ignore) - 1;
  mc_ignore_region_t region;

  while(start <= end){
    cursor = (start + end) / 2;
    region = (mc_ignore_region_t)xbt_dynar_get_as(mmalloc_ignore, cursor, mc_ignore_region_t);
    if(region->block == block){
      if(region->fragment == fragment)
        return 1;
      if(region->fragment < fragment)
        start = cursor + 1;
      if(region->fragment > fragment)
        end = cursor - 1;
    }
    if(region->block < block)
      start = cursor + 1;
    if(region->block > block)
      end = cursor - 1; 
  }

  return 0;
}

static size_t heap_comparison_ignore(void *address){
  unsigned int cursor = 0;
  int start = 0;
  int end = xbt_dynar_length(mmalloc_ignore) - 1;
  mc_ignore_region_t region;

  while(start <= end){
    cursor = (start + end) / 2;
    region = (mc_ignore_region_t)xbt_dynar_get_as(mmalloc_ignore, cursor, mc_ignore_region_t);
    if(region->address == address)
      return region->size;
    if(region->address < address)
      start = cursor + 1;
    if(region->address > address)
      end = cursor - 1;   
  }

  return 0;
}


static int compare_area(void *area1, void* area2, size_t size, xbt_dynar_t previous, int check_ignore){

  size_t i = 0, pointer_align = 0, ignore1 = 0, ignore2 = 0;
  void *address_pointed1, *address_pointed2, *addr_block_pointed1, *addr_block_pointed2, *addr_frag_pointed1, *addr_frag_pointed2;
  size_t block_pointed1, block_pointed2, frag_pointed1, frag_pointed2;
  int res_compare;
  void *current_area1, *current_area2;
 
  while(i<size){

    if(check_ignore){

      current_area1 = (char*)((xbt_mheap_t)s_heap)->heapbase + ((((char *)area1) + i) - (char *)heapbase1);
      if((ignore1 = heap_comparison_ignore(current_area1)) > 0){
        current_area2 = (char*)((xbt_mheap_t)s_heap)->heapbase + ((((char *)area2) + i) - (char *)heapbase2);
        if((ignore2 = heap_comparison_ignore(current_area2))  == ignore1){
          i = i + ignore2;
          ignore_done++;
          continue;
        }
      }

    }
   
    if(memcmp(((char *)area1) + i, ((char *)area2) + i, 1) != 0){

      /* Check pointer difference */
      pointer_align = (i / sizeof(void*)) * sizeof(void*);
      address_pointed1 = *((void **)((char *)area1 + pointer_align));
      address_pointed2 = *((void **)((char *)area2 + pointer_align));

      /* Get pointed blocks number */ 
      block_pointed1 = ((char*)address_pointed1 - (char*)((xbt_mheap_t)s_heap)->heapbase) / BLOCKSIZE + 1;
      block_pointed2 = ((char*)address_pointed2 - (char*)((xbt_mheap_t)s_heap)->heapbase) / BLOCKSIZE + 1;

      /* Check if valid blocks number */
      if((char *)address_pointed1 < (char*)((xbt_mheap_t)s_heap)->heapbase || block_pointed1 > heapsize1 || block_pointed1 < 1 || (char *)address_pointed2 < (char*)((xbt_mheap_t)s_heap)->heapbase || block_pointed2 > heapsize2 || block_pointed2 < 1)
        return 1;

      if(heapinfo1[block_pointed1].type == heapinfo2[block_pointed2].type){ /* Same type of block (large or fragmented) */

        addr_block_pointed1 = ((void*) (((ADDR2UINT(block_pointed1)) - 1) * BLOCKSIZE + (char*)heapbase1));
        addr_block_pointed2 = ((void*) (((ADDR2UINT(block_pointed2)) - 1) * BLOCKSIZE + (char*)heapbase2));
        
        if(heapinfo1[block_pointed1].type == 0){ /* Large block */

          if(heapinfo1[block_pointed1].busy_block.size != heapinfo2[block_pointed2].busy_block.size){
            return 1;
          }

          if(heapinfo1[block_pointed1].busy_block.busy_size != heapinfo2[block_pointed2].busy_block.busy_size){
            return 1;
          }

          if(add_heap_area_pair(previous, block_pointed1, -1, block_pointed2, -1)){

            if(ignore_done < xbt_dynar_length(mmalloc_ignore)){
              if(in_mmalloc_ignore(block_pointed1, -1))
                res_compare = compare_area(addr_block_pointed1, addr_block_pointed2, heapinfo1[block_pointed1].busy_block.busy_size, previous, 1);
              else
                res_compare = compare_area(addr_block_pointed1, addr_block_pointed2, heapinfo1[block_pointed1].busy_block.busy_size, previous, 0);
            }else{
              res_compare = compare_area(addr_block_pointed1, addr_block_pointed2, heapinfo1[block_pointed1].busy_block.busy_size, previous, 0);
            }
            
            if(res_compare)    
              return 1;
           
          }
          
        }else{ /* Fragmented block */

          /* Get pointed fragments number */ 
          frag_pointed1 = ((uintptr_t) (ADDR2UINT (address_pointed1) % (BLOCKSIZE))) >> heapinfo1[block_pointed1].type;
          frag_pointed2 = ((uintptr_t) (ADDR2UINT (address_pointed2) % (BLOCKSIZE))) >> heapinfo2[block_pointed2].type;
         
          if(heapinfo1[block_pointed1].busy_frag.frag_size[frag_pointed1] != heapinfo2[block_pointed2].busy_frag.frag_size[frag_pointed2]) /* Different size_used */    
            return 1;
            
          addr_frag_pointed1 = (void*) ((char *)addr_block_pointed1 + (frag_pointed1 << heapinfo1[block_pointed1].type));
          addr_frag_pointed2 = (void*) ((char *)addr_block_pointed2 + (frag_pointed2 << heapinfo2[block_pointed2].type));

          if(add_heap_area_pair(previous, block_pointed1, frag_pointed1, block_pointed2, frag_pointed2)){

            if(ignore_done < xbt_dynar_length(mmalloc_ignore)){
              if(in_mmalloc_ignore(block_pointed1, frag_pointed1))
                res_compare = compare_area(addr_frag_pointed1, addr_frag_pointed2, heapinfo1[block_pointed1].busy_frag.frag_size[frag_pointed1], previous, 1);
              else
                res_compare = compare_area(addr_frag_pointed1, addr_frag_pointed2, heapinfo1[block_pointed1].busy_frag.frag_size[frag_pointed1], previous, 0);
            }else{
              res_compare = compare_area(addr_frag_pointed1, addr_frag_pointed2, heapinfo1[block_pointed1].busy_frag.frag_size[frag_pointed1], previous, 0);
            }

            if(res_compare)
              return 1;
           
          }
          
        }
          
      }else{

        if((heapinfo1[block_pointed1].type > 0) && (heapinfo2[block_pointed2].type > 0)){
          
          addr_block_pointed1 = ((void*) (((ADDR2UINT(block_pointed1)) - 1) * BLOCKSIZE + (char*)heapbase1));
          addr_block_pointed2 = ((void*) (((ADDR2UINT(block_pointed2)) - 1) * BLOCKSIZE + (char*)heapbase2));
       
          frag_pointed1 = ((uintptr_t) (ADDR2UINT (address_pointed1) % (BLOCKSIZE))) >> heapinfo1[block_pointed1].type;
          frag_pointed2 = ((uintptr_t) (ADDR2UINT (address_pointed2) % (BLOCKSIZE))) >> heapinfo2[block_pointed2].type;

          if(heapinfo1[block_pointed1].busy_frag.frag_size[frag_pointed1] != heapinfo2[block_pointed2].busy_frag.frag_size[frag_pointed2]) /* Different size_used */    
            return 1;

          addr_frag_pointed1 = (void*) ((char *)addr_block_pointed1 + (frag_pointed1 << heapinfo1[block_pointed1].type));
          addr_frag_pointed2 = (void*) ((char *)addr_block_pointed2 + (frag_pointed2 << heapinfo2[block_pointed2].type));

          if(add_heap_area_pair(previous, block_pointed1, frag_pointed1, block_pointed2, frag_pointed2)){

            if(ignore_done < xbt_dynar_length(mmalloc_ignore)){
              if(in_mmalloc_ignore(block_pointed1, frag_pointed1))
                res_compare = compare_area(addr_frag_pointed1, addr_frag_pointed2, heapinfo1[block_pointed1].busy_frag.frag_size[frag_pointed1], previous, 1);
              else
                res_compare = compare_area(addr_frag_pointed1, addr_frag_pointed2, heapinfo1[block_pointed1].busy_frag.frag_size[frag_pointed1], previous, 0);
            }else{
              res_compare = compare_area(addr_frag_pointed1, addr_frag_pointed2, heapinfo1[block_pointed1].busy_frag.frag_size[frag_pointed1], previous, 0);
            }
            
            if(res_compare)
              return 1;
           
          }

        }else{
          return 1;
        }

      }

      i = pointer_align + sizeof(void *);
      
    }else{

      i++;

    }
  }

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
  heap_area_pair_t current_pair;

  xbt_dynar_foreach(list, cursor, current_pair){
    if(current_pair->block1 == block1 && current_pair->block2 == block2 && current_pair->fragment1 == fragment1 && current_pair->fragment2 == fragment2)
      return 0; 
  }
  
  return 1;
}

static void match_equals(xbt_dynar_t list){

  unsigned int cursor = 0;
  heap_area_pair_t current_pair;

  xbt_dynar_foreach(list, cursor, current_pair){
    if(current_pair->fragment1 != -1){
      heapinfo1[current_pair->block1].busy_frag.equal_to[current_pair->fragment1] = 1;
      heapinfo2[current_pair->block2].busy_frag.equal_to[current_pair->fragment2] = 1;
    }else{
      heapinfo1[current_pair->block1].busy_block.equal_to = 1;
      heapinfo2[current_pair->block2].busy_block.equal_to = 1;
    }
  }

}

#ifndef max
	#define max( a, b ) ( ((a) > (b)) ? (a) : (b) )
#endif

int mmalloc_linear_compare_heap(xbt_mheap_t heap1, xbt_mheap_t heap2){

  if(heap1 == NULL && heap1 == NULL){
    XBT_DEBUG("Malloc descriptors null");
    return 0;
  }

  if(heap1->heaplimit != heap2->heaplimit){
    XBT_DEBUG("Different limit of valid info table indices");
    return 1;
  }

  /* Heap information */
  heaplimit = ((struct mdesc *)heap1)->heaplimit;

  s_heap = (char *)mmalloc_get_current_heap() - STD_HEAP_SIZE - getpagesize();

  heapbase1 = (char *)heap1 + BLOCKSIZE;
  heapbase2 = (char *)heap2 + BLOCKSIZE;

  heapinfo1 = (malloc_info *)((char *)heap1 + ((uintptr_t)((char *)heap1->heapinfo - (char *)s_heap)));
  heapinfo2 = (malloc_info *)((char *)heap2 + ((uintptr_t)((char *)heap2->heapinfo - (char *)s_heap)));

  heapsize1 = heap1->heapsize;
  heapsize2 = heap2->heapsize;

  /* Start comparison */
  size_t i, j, k;
  void *addr_block1, *addr_block2, *addr_frag1, *addr_frag2;

  int distance = 0;

  /* Check busy blocks*/

  i = 1;

  while(i <= heaplimit){

    addr_block1 = ((void*) (((ADDR2UINT(i)) - 1) * BLOCKSIZE + (char*)heapbase1));
    addr_block2 = ((void*) (((ADDR2UINT(i)) - 1) * BLOCKSIZE + (char*)heapbase2));

    if(heapinfo1[i].type != heapinfo2[i].type){
  
      distance += BLOCKSIZE;
      XBT_DEBUG("Different type of blocks (%zu) : %d - %d -> distance = %d", i, heapinfo1[i].type, heapinfo2[i].type, distance);
      i++;
    
    }else{

      if(heapinfo1[i].type == -1){ /* Free block */
        i++;
        continue;
      }

      if(heapinfo1[i].type == 0){ /* Large block */
       
        if(heapinfo1[i].busy_block.size != heapinfo2[i].busy_block.size){
          distance += BLOCKSIZE * max(heapinfo1[i].busy_block.size, heapinfo2[i].busy_block.size);
          i += max(heapinfo1[i].busy_block.size, heapinfo2[i].busy_block.size);
          XBT_DEBUG("Different larger of cluster at block %zu : %zu - %zu -> distance = %d", i, heapinfo1[i].busy_block.size, heapinfo2[i].busy_block.size, distance);
          continue;
        }

        /*if(heapinfo1[i].busy_block.busy_size != heapinfo2[i].busy_block.busy_size){
          distance += max(heapinfo1[i].busy_block.busy_size, heapinfo2[i].busy_block.busy_size);
          i += max(heapinfo1[i].busy_block.size, heapinfo2[i].busy_block.size);
          XBT_DEBUG("Different size used oin large cluster at block %zu : %zu - %zu -> distance = %d", i, heapinfo1[i].busy_block.busy_size, heapinfo2[i].busy_block.busy_size, distance);
          continue;
          }*/

        k = 0;

        //while(k < (heapinfo1[i].busy_block.busy_size)){
        while(k < heapinfo1[i].busy_block.size * BLOCKSIZE){
          if(memcmp((char *)addr_block1 + k, (char *)addr_block2 + k, 1) != 0){
            distance ++;
          }
          k++;
        } 

        i++;

      }else { /* Fragmented block */

        for(j=0; j < (size_t) (BLOCKSIZE >> heapinfo1[i].type); j++){

          addr_frag1 = (void*) ((char *)addr_block1 + (j << heapinfo1[i].type));
          addr_frag2 = (void*) ((char *)addr_block2 + (j << heapinfo2[i].type));

          if(heapinfo1[i].busy_frag.frag_size[j] == 0 && heapinfo2[i].busy_frag.frag_size[j] == 0){
            continue;
          }
          
          
          /*if(heapinfo1[i].busy_frag.frag_size[j] != heapinfo2[i].busy_frag.frag_size[j]){
            distance += max(heapinfo1[i].busy_frag.frag_size[j], heapinfo2[i].busy_frag.frag_size[j]);
            XBT_DEBUG("Different size used in fragment %zu in block %zu : %d - %d -> distance = %d", j, i, heapinfo1[i].busy_frag.frag_size[j], heapinfo2[i].busy_frag.frag_size[j], distance); 
            continue;
            }*/
   
          k=0;

          //while(k < max(heapinfo1[i].busy_frag.frag_size[j], heapinfo2[i].busy_frag.frag_size[j])){
          while(k < (BLOCKSIZE / (BLOCKSIZE >> heapinfo1[i].type))){
            if(memcmp((char *)addr_frag1 + k, (char *)addr_frag2 + k, 1) != 0){
              distance ++;
            }
            k++;
          }

        }

        i++;

      }
      
    }

  }

  return distance;
  
}

