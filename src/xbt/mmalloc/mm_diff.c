/* mm_diff - Memory snapshooting and comparison                             */

/* Copyright (c) 2008-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex_interface.h" /* internals of backtrace setup */
#include "xbt/str.h"
#include "mc/mc.h"
#include "xbt/mmalloc.h"
#include "mc/datatypes.h"
#include "mc/mc_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mm_diff, xbt,
                                "Logging specific to mm_diff in mmalloc");

xbt_dynar_t mc_heap_comparison_ignore;
xbt_dynar_t stacks_areas;
void *maestro_stack_start, *maestro_stack_end;


/********************************* Backtrace ***********************************/
/******************************************************************************/

static void mmalloc_backtrace_block_display(void* heapinfo, int block){

  /* xbt_ex_t e; */

  /* if (((malloc_info *)heapinfo)[block].busy_block.bt_size == 0) { */
  /*   fprintf(stderr, "No backtrace available for that block, sorry.\n"); */
  /*   return; */
  /* } */

  /* memcpy(&e.bt,&(((malloc_info *)heapinfo)[block].busy_block.bt),sizeof(void*)*XBT_BACKTRACE_SIZE); */
  /* e.used = ((malloc_info *)heapinfo)[block].busy_block.bt_size; */

  /* xbt_ex_setup_backtrace(&e); */
  /* if (e.used == 0) { */
  /*   fprintf(stderr, "(backtrace not set)\n"); */
  /* } else if (e.bt_strings == NULL) { */
  /*   fprintf(stderr, "(backtrace not ready to be computed. %s)\n",xbt_binary_name?"Dunno why":"xbt_binary_name not setup yet"); */
  /* } else { */
  /*   int i; */

  /*   fprintf(stderr, "Backtrace of where the block %d was malloced (%d frames):\n", block ,e.used); */
  /*   for (i = 0; i < e.used; i++)       /\* no need to display "xbt_backtrace_display" *\/{ */
  /*     fprintf(stderr, "%d ---> %s\n",i, e.bt_strings[i] + 4); */
  /*   } */
  /* } */
}

static void mmalloc_backtrace_fragment_display(void* heapinfo, int block, int frag){

  /* xbt_ex_t e; */

  /* memcpy(&e.bt,&(((malloc_info *)heapinfo)[block].busy_frag.bt[frag]),sizeof(void*)*XBT_BACKTRACE_SIZE); */
  /* e.used = XBT_BACKTRACE_SIZE; */

  /* xbt_ex_setup_backtrace(&e); */
  /* if (e.used == 0) { */
  /*   fprintf(stderr, "(backtrace not set)\n"); */
  /* } else if (e.bt_strings == NULL) { */
  /*   fprintf(stderr, "(backtrace not ready to be computed. %s)\n",xbt_binary_name?"Dunno why":"xbt_binary_name not setup yet"); */
  /* } else { */
  /*   int i; */

  /*   fprintf(stderr, "Backtrace of where the fragment %d in block %d was malloced (%d frames):\n", frag, block ,e.used); */
  /*   for (i = 0; i < e.used; i++)       /\* no need to display "xbt_backtrace_display" *\/{ */
  /*     fprintf(stderr, "%d ---> %s\n",i, e.bt_strings[i] + 4); */
  /*   } */
  /* } */

}

static void mmalloc_backtrace_display(void *addr){

  /* size_t block, frag_nb; */
  /* int type; */
  
  /* xbt_mheap_t heap = __mmalloc_current_heap ?: (xbt_mheap_t) mmalloc_preinit(); */

  /* block = (((char*) (addr) - (char*) heap -> heapbase) / BLOCKSIZE + 1); */

  /* type = heap->heapinfo[block].type; */

  /* switch(type){ */
  /* case -1 : /\* Free block *\/ */
  /*   fprintf(stderr, "Asked to display the backtrace of a block that is free. I'm puzzled\n"); */
  /*   xbt_abort(); */
  /*   break;  */
  /* case 0: /\* Large block *\/ */
  /*   mmalloc_backtrace_block_display(heap->heapinfo, block); */
  /*   break; */
  /* default: /\* Fragmented block *\/ */
  /*   frag_nb = RESIDUAL(addr, BLOCKSIZE) >> type; */
  /*   if(heap->heapinfo[block].busy_frag.frag_size[frag_nb] == -1){ */
  /*     fprintf(stderr , "Asked to display the backtrace of a fragment that is free. I'm puzzled\n"); */
  /*     xbt_abort(); */
  /*   } */
  /*   mmalloc_backtrace_fragment_display(heap->heapinfo, block, frag_nb); */
  /*   break; */
  /* } */
}


static int compare_backtrace(int b1, int f1, int b2, int f2){
  /*int i = 0;
  if(f1 != -1){
    for(i=0; i< XBT_BACKTRACE_SIZE; i++){
      if(heapinfo1[b1].busy_frag.bt[f1][i] != heapinfo2[b2].busy_frag.bt[f2][i]){
        //mmalloc_backtrace_fragment_display((void*)heapinfo1, b1, f1);
        //mmalloc_backtrace_fragment_display((void*)heapinfo2, b2, f2);
        return 1;
      }
    }
  }else{
    for(i=0; i< heapinfo1[b1].busy_block.bt_size; i++){
      if(heapinfo1[b1].busy_block.bt[i] != heapinfo2[b2].busy_block.bt[i]){
        //mmalloc_backtrace_block_display((void*)heapinfo1, b1);
        //mmalloc_backtrace_block_display((void*)heapinfo2, b2);
        return 1;
      }
    }
    }*/
  return 0;
}


/*********************************** Heap comparison ***********************************/
/***************************************************************************************/

typedef char* type_name;

struct s_mm_diff {
  void *s_heap, *heapbase1, *heapbase2;
  malloc_info *heapinfo1, *heapinfo2;
  size_t heaplimit;
  // Number of blocks in the heaps:
  size_t heapsize1, heapsize2;
  xbt_dynar_t to_ignore1, to_ignore2;
  heap_area_t **equals_to1, **equals_to2;
  type_name **types1, **types2;
};

__thread struct s_mm_diff* mm_diff_info = NULL;

/*********************************** Free functions ************************************/

static void heap_area_pair_free(heap_area_pair_t pair){
  xbt_free(pair);
  pair = NULL;
}

static void heap_area_pair_free_voidp(void *d){
  heap_area_pair_free((heap_area_pair_t) * (void **) d);
}

static void heap_area_free(heap_area_t area){
  xbt_free(area);
  area = NULL;
}

/************************************************************************************/

static heap_area_t new_heap_area(int block, int fragment){
  heap_area_t area = NULL;
  area = xbt_new0(s_heap_area_t, 1);
  area->block = block;
  area->fragment = fragment;
  return area;
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

static ssize_t heap_comparison_ignore_size(xbt_dynar_t ignore_list, void *address){

  unsigned int cursor = 0;
  int start = 0;
  int end = xbt_dynar_length(ignore_list) - 1;
  mc_heap_ignore_region_t region;

  while(start <= end){
    cursor = (start + end) / 2;
    region = (mc_heap_ignore_region_t)xbt_dynar_get_as(ignore_list, cursor, mc_heap_ignore_region_t);
    if(region->address == address)
      return region->size;
    if(region->address < address)
      start = cursor + 1;
    if(region->address > address)
      end = cursor - 1;   
  }

  return -1;
}

static int is_stack(void *address){
  unsigned int cursor = 0;
  stack_region_t stack;

  xbt_dynar_foreach(stacks_areas, cursor, stack){
    if(address == stack->address)
      return 1;
  }

  return 0;
}

static int is_block_stack(int block){
  unsigned int cursor = 0;
  stack_region_t stack;

  xbt_dynar_foreach(stacks_areas, cursor, stack){
    if(block == stack->block)
      return 1;
  }

  return 0;
}

static void match_equals(struct s_mm_diff *state, xbt_dynar_t list){

  unsigned int cursor = 0;
  heap_area_pair_t current_pair;
  heap_area_t previous_area;

  xbt_dynar_foreach(list, cursor, current_pair){

    if(current_pair->fragment1 != -1){

      if(state->equals_to1[current_pair->block1][current_pair->fragment1] != NULL){
        previous_area = state->equals_to1[current_pair->block1][current_pair->fragment1];
        heap_area_free(state->equals_to2[previous_area->block][previous_area->fragment]);
        state->equals_to2[previous_area->block][previous_area->fragment] = NULL;
        heap_area_free(previous_area);
      }
      if(state->equals_to2[current_pair->block2][current_pair->fragment2] != NULL){
        previous_area = state->equals_to2[current_pair->block2][current_pair->fragment2];
        heap_area_free(state->equals_to1[previous_area->block][previous_area->fragment]);
        state->equals_to1[previous_area->block][previous_area->fragment] = NULL;
        heap_area_free(previous_area);
      }

      state->equals_to1[current_pair->block1][current_pair->fragment1] = new_heap_area(current_pair->block2, current_pair->fragment2);
      state->equals_to2[current_pair->block2][current_pair->fragment2] = new_heap_area(current_pair->block1, current_pair->fragment1);
      
    }else{

      if(state->equals_to1[current_pair->block1][0] != NULL){
        previous_area = state->equals_to1[current_pair->block1][0];
        heap_area_free(state->equals_to2[previous_area->block][0]);
        state->equals_to2[previous_area->block][0] = NULL;
        heap_area_free(previous_area);
      }
      if(state->equals_to2[current_pair->block2][0] != NULL){
        previous_area = state->equals_to2[current_pair->block2][0];
        heap_area_free(state->equals_to1[previous_area->block][0]);
        state->equals_to1[previous_area->block][0] = NULL;
        heap_area_free(previous_area);
      }

      state->equals_to1[current_pair->block1][0] = new_heap_area(current_pair->block2, current_pair->fragment2);
      state->equals_to2[current_pair->block2][0] = new_heap_area(current_pair->block1, current_pair->fragment1);

    }

  }
}

/** Check whether two blocks are known to be matching
 *
 *  @param state  State used
 *  @param b1     Block of state 1
 *  @param b2     Block of state 2
 *  @return       if the blocks are known to be matching
 */
static int equal_blocks(struct s_mm_diff *state, int b1, int b2){
  
  if(state->equals_to1[b1][0]->block == b2 && state->equals_to2[b2][0]->block == b1)
    return 1;

  return 0;
}

/** Check whether two fragments are known to be matching
 *
 *  @param state  State used
 *  @param b1     Block of state 1
 *  @param f1     Fragment of state 1
 *  @param b2     Block of state 2
 *  @param f2     Fragment of state 2
 *  @return       if the fragments are known to be matching
 */
static int equal_fragments(struct s_mm_diff *state, int b1, int f1, int b2, int f2){
  
  if(state->equals_to1[b1][f1]->block == b2
    && state->equals_to1[b1][f1]->fragment == f2
    && state->equals_to2[b2][f2]->block == b1
    && state->equals_to2[b2][f2]->fragment == f1)
    return 1;

  return 0;
}

int init_heap_information(xbt_mheap_t heap1, xbt_mheap_t heap2, xbt_dynar_t i1, xbt_dynar_t i2){
  if(mm_diff_info==NULL) {
    mm_diff_info = xbt_new0(struct s_mm_diff, 1);
  }
  struct s_mm_diff *state = mm_diff_info;

  if((((struct mdesc *)heap1)->heaplimit != ((struct mdesc *)heap2)->heaplimit)
    || ((((struct mdesc *)heap1)->heapsize != ((struct mdesc *)heap2)->heapsize) ))
    return -1;

  int i, j;

  state->heaplimit = ((struct mdesc *)heap1)->heaplimit;

  state->s_heap = (char *)mmalloc_get_current_heap() - STD_HEAP_SIZE - getpagesize();

  state->heapbase1 = (char *)heap1 + BLOCKSIZE;
  state->heapbase2 = (char *)heap2 + BLOCKSIZE;

  state->heapinfo1 = (malloc_info *)((char *)heap1 + ((uintptr_t)((char *)((struct mdesc *)heap1)->heapinfo - (char *)state->s_heap)));
  state->heapinfo2 = (malloc_info *)((char *)heap2 + ((uintptr_t)((char *)((struct mdesc *)heap2)->heapinfo - (char *)state->s_heap)));

  state->heapsize1 = heap1->heapsize;
  state->heapsize2 = heap2->heapsize;

  state->to_ignore1 = i1;
  state-> to_ignore2 = i2;

  state->equals_to1 = malloc(state->heaplimit * sizeof(heap_area_t *));
  state->types1 = malloc(state->heaplimit * sizeof(type_name *));
  for(i=0; i<=state->heaplimit; i++){
    state->equals_to1[i] = malloc(MAX_FRAGMENT_PER_BLOCK * sizeof(heap_area_t));
    state->types1[i] = malloc(MAX_FRAGMENT_PER_BLOCK * sizeof(type_name));
    for(j=0; j<MAX_FRAGMENT_PER_BLOCK; j++){
      state->equals_to1[i][j] = NULL;
      state->types1[i][j] = NULL;
    }      
  }

  state->equals_to2 = malloc(state->heaplimit * sizeof(heap_area_t *));
  state->types2 = malloc(state->heaplimit * sizeof(type_name *));
  for(i=0; i<=state->heaplimit; i++){
    state->equals_to2[i] = malloc(MAX_FRAGMENT_PER_BLOCK * sizeof(heap_area_t));
    state->types2[i] = malloc(MAX_FRAGMENT_PER_BLOCK * sizeof(type_name));
    for(j=0; j<MAX_FRAGMENT_PER_BLOCK; j++){
      state->equals_to2[i][j] = NULL;
      state->types2[i][j] = NULL;
    }
  }

  if(MC_is_active()){
    MC_ignore_global_variable("mm_diff_info");
  }

  return 0;

}

void reset_heap_information(){

  struct s_mm_diff *state = mm_diff_info;

  size_t i = 0, j;

  for(i=0; i<=state->heaplimit; i++){
    for(j=0; j<MAX_FRAGMENT_PER_BLOCK;j++){
      heap_area_free(state->equals_to1[i][j]);
      state->equals_to1[i][j] = NULL;
      heap_area_free(state->equals_to2[i][j]);
      state-> equals_to2[i][j] = NULL;
      xbt_free(state->types1[i][j]);
      state->types1[i][j] = NULL;
      xbt_free(state->types2[i][j]);
      state->types2[i][j] = NULL;
    }
    free(state->equals_to1[i]);
    free(state->equals_to2[i]);
    free(state->types1[i]);
    free(state->types2[i]);
  }

  free(state->equals_to1);
  free(state->equals_to2);
  free(state->types1);
  free(state->types2);

  state->s_heap = NULL, state->heapbase1 = NULL, state->heapbase2 = NULL;
  state->heapinfo1 = NULL, state->heapinfo2 = NULL;
  state->heaplimit = 0, state->heapsize1 = 0, state->heapsize2 = 0;
  state->to_ignore1 = NULL, state->to_ignore2 = NULL;
  state->equals_to1 = NULL, state->equals_to2 = NULL;
  state->types1 = NULL, state->types2 = NULL;

}

int mmalloc_compare_heap(mc_snapshot_t snapshot1, mc_snapshot_t snapshot2, xbt_mheap_t heap1, xbt_mheap_t heap2, mc_object_info_t info, mc_object_info_t other_info){

  struct s_mm_diff *state = mm_diff_info;

  if(heap1 == NULL && heap2 == NULL){
    XBT_DEBUG("Malloc descriptors null");
    return 0;
  }

  /* Start comparison */
  size_t i1, i2, j1, j2, k;
  void *addr_block1, *addr_block2, *addr_frag1, *addr_frag2;
  int nb_diff1 = 0, nb_diff2 = 0;

  xbt_dynar_t previous = xbt_dynar_new(sizeof(heap_area_pair_t), heap_area_pair_free_voidp);

  int equal, res_compare = 0;

  /* Check busy blocks*/

  i1 = 1;

  while(i1 <= state->heaplimit){

    if(state->heapinfo1[i1].type == -1){ /* Free block */
      i1++;
      continue;
    }

    addr_block1 = ((void*) (((ADDR2UINT(i1)) - 1) * BLOCKSIZE + (char*)((xbt_mheap_t)state->s_heap)->heapbase));

    if(state->heapinfo1[i1].type == 0){  /* Large block */
      
      if(is_stack(addr_block1)){
        for(k=0; k < state->heapinfo1[i1].busy_block.size; k++)
          state->equals_to1[i1+k][0] = new_heap_area(i1, -1);
        for(k=0; k < state->heapinfo2[i1].busy_block.size; k++)
          state->equals_to2[i1+k][0] = new_heap_area(i1, -1);
        i1 += state->heapinfo1[i1].busy_block.size;
        continue;
      }

      if(state->equals_to1[i1][0] != NULL){
        i1++;
        continue;
      }
    
      i2 = 1;
      equal = 0;
      res_compare = 0;
  
      /* Try first to associate to same block in the other heap */
      if(state->heapinfo2[i1].type == state->heapinfo1[i1].type){

        if(state->equals_to2[i1][0] == NULL){

          addr_block2 = ((void*) (((ADDR2UINT(i1)) - 1) * BLOCKSIZE + (char*)((xbt_mheap_t)state->s_heap)->heapbase));
        
          res_compare = compare_heap_area(addr_block1, addr_block2, snapshot1, snapshot2, NULL, info, other_info, NULL, 0);
        
          if(res_compare != 1){
            for(k=1; k < state->heapinfo2[i1].busy_block.size; k++)
              state->equals_to2[i1+k][0] = new_heap_area(i1, -1);
            for(k=1; k < state->heapinfo1[i1].busy_block.size; k++)
              state->equals_to1[i1+k][0] = new_heap_area(i1, -1);
            equal = 1;
            i1 += state->heapinfo1[i1].busy_block.size;
          }
        
          xbt_dynar_reset(previous);
        
        }
        
      }

      while(i2 <= state->heaplimit && !equal){

        addr_block2 = ((void*) (((ADDR2UINT(i2)) - 1) * BLOCKSIZE + (char*)((xbt_mheap_t)state->s_heap)->heapbase));
           
        if(i2 == i1){
          i2++;
          continue;
        }

        if(state->heapinfo2[i2].type != 0){
          i2++;
          continue;
        }
    
        if(state->equals_to2[i2][0] != NULL){
          i2++;
          continue;
        }
          
        res_compare = compare_heap_area(addr_block1, addr_block2, snapshot1, snapshot2, NULL, info, other_info, NULL, 0);
        
        if(res_compare != 1 ){
          for(k=1; k < state->heapinfo2[i2].busy_block.size; k++)
            state->equals_to2[i2+k][0] = new_heap_area(i1, -1);
          for(k=1; k < state->heapinfo1[i1].busy_block.size; k++)
            state->equals_to1[i1+k][0] = new_heap_area(i2, -1);
          equal = 1;
          i1 += state->heapinfo1[i1].busy_block.size;
        }

        xbt_dynar_reset(previous);

        i2++;

      }

      if(!equal){
        XBT_DEBUG("Block %zu not found (size_used = %zu, addr = %p)", i1, state->heapinfo1[i1].busy_block.busy_size, addr_block1);
        i1 = state->heaplimit + 1;
        nb_diff1++;
          //i1++;
      }
      
    }else{ /* Fragmented block */

      for(j1=0; j1 < (size_t) (BLOCKSIZE >> state->heapinfo1[i1].type); j1++){

        if(state->heapinfo1[i1].busy_frag.frag_size[j1] == -1) /* Free fragment */
          continue;

        if(state->equals_to1[i1][j1] != NULL)
          continue;

        addr_frag1 = (void*) ((char *)addr_block1 + (j1 << state->heapinfo1[i1].type));

        i2 = 1;
        equal = 0;
        
        /* Try first to associate to same fragment in the other heap */
        if(state->heapinfo2[i1].type == state->heapinfo1[i1].type){

          if(state->equals_to2[i1][j1] == NULL){

            addr_block2 = ((void*) (((ADDR2UINT(i1)) - 1) * BLOCKSIZE + (char*)((xbt_mheap_t)state->s_heap)->heapbase));
            addr_frag2 = (void*) ((char *)addr_block2 + (j1 << ((xbt_mheap_t)state->s_heap)->heapinfo[i1].type));

            res_compare = compare_heap_area(addr_frag1, addr_frag2, snapshot1, snapshot2, NULL, info, other_info, NULL, 0);

            if(res_compare !=  1)
              equal = 1;
        
            xbt_dynar_reset(previous);

          }

        }

        while(i2 <= state->heaplimit && !equal){

          if(state->heapinfo2[i2].type <= 0){
            i2++;
            continue;
          }

          for(j2=0; j2 < (size_t) (BLOCKSIZE >> state->heapinfo2[i2].type); j2++){

            if(i2 == i1 && j2 == j1)
              continue;
           
            if(state->equals_to2[i2][j2] != NULL)
              continue;
                          
            addr_block2 = ((void*) (((ADDR2UINT(i2)) - 1) * BLOCKSIZE + (char*)((xbt_mheap_t)state->s_heap)->heapbase));
            addr_frag2 = (void*) ((char *)addr_block2 + (j2 <<((xbt_mheap_t)state->s_heap)->heapinfo[i2].type));

            res_compare = compare_heap_area(addr_frag1, addr_frag2, snapshot2, snapshot2, NULL, info, other_info, NULL, 0);
            
            if(res_compare != 1){
              equal = 1;
              xbt_dynar_reset(previous);
              break;
            }

            xbt_dynar_reset(previous);

          }

          i2++;

        }

        if(!equal){
          XBT_DEBUG("Block %zu, fragment %zu not found (size_used = %zd, address = %p)\n", i1, j1, state->heapinfo1[i1].busy_frag.frag_size[j1], addr_frag1);
          i2 = state->heaplimit + 1;
          i1 = state->heaplimit + 1;
          nb_diff1++;
          break;
        }

      }

      i1++;
      
    }

  }

  /* All blocks/fragments are equal to another block/fragment ? */
  size_t i = 1, j = 0;
  void *real_addr_frag1 = NULL, *real_addr_block1 = NULL, *real_addr_block2 = NULL, *real_addr_frag2 = NULL;
 
  while(i<=state->heaplimit){
    if(state->heapinfo1[i].type == 0){
      if(i1 == state->heaplimit){
        if(state->heapinfo1[i].busy_block.busy_size > 0){
          if(state->equals_to1[i][0] == NULL){
            if(XBT_LOG_ISENABLED(mm_diff, xbt_log_priority_debug)){
              addr_block1 = ((void*) (((ADDR2UINT(i)) - 1) * BLOCKSIZE + (char*)state->heapbase1));
              XBT_DEBUG("Block %zu (%p) not found (size used = %zu)", i, addr_block1, state->heapinfo1[i].busy_block.busy_size);
              //mmalloc_backtrace_block_display((void*)heapinfo1, i);
            }
            nb_diff1++;
          }
        }
      }
    }
    if(state->heapinfo1[i].type > 0){
      addr_block1 = ((void*) (((ADDR2UINT(i)) - 1) * BLOCKSIZE + (char*)state->heapbase1));
      real_addr_block1 =  ((void*) (((ADDR2UINT(i)) - 1) * BLOCKSIZE + (char*)((struct mdesc *)state->s_heap)->heapbase));
      for(j=0; j < (size_t) (BLOCKSIZE >> state->heapinfo1[i].type); j++){
        if(i1== state->heaplimit){
          if(state->heapinfo1[i].busy_frag.frag_size[j] > 0){
            if(state->equals_to1[i][j] == NULL){
              if(XBT_LOG_ISENABLED(mm_diff, xbt_log_priority_debug)){
                addr_frag1 = (void*) ((char *)addr_block1 + (j << state->heapinfo1[i].type));
                real_addr_frag1 = (void*) ((char *)real_addr_block1 + (j << ((struct mdesc *)state->s_heap)->heapinfo[i].type));
                XBT_DEBUG("Block %zu, Fragment %zu (%p - %p) not found (size used = %zd)", i, j, addr_frag1, real_addr_frag1, state->heapinfo1[i].busy_frag.frag_size[j]);
                //mmalloc_backtrace_fragment_display((void*)heapinfo1, i, j);
              }
              nb_diff1++;
            }
          }
        }
      }
    }
    i++; 
  }

  if(i1 == state->heaplimit)
    XBT_DEBUG("Number of blocks/fragments not found in heap1 : %d", nb_diff1);

  i = 1;

  while(i<=state->heaplimit){
    if(state->heapinfo2[i].type == 0){
      if(i1 == state->heaplimit){
        if(state->heapinfo2[i].busy_block.busy_size > 0){
          if(state->equals_to2[i][0] == NULL){
            if(XBT_LOG_ISENABLED(mm_diff, xbt_log_priority_debug)){
              addr_block2 = ((void*) (((ADDR2UINT(i)) - 1) * BLOCKSIZE + (char*)state->heapbase2));
              XBT_DEBUG("Block %zu (%p) not found (size used = %zu)", i, addr_block2, state->heapinfo2[i].busy_block.busy_size);
              //mmalloc_backtrace_block_display((void*)heapinfo2, i);
            }
            nb_diff2++;
          }
        }
      }
    }
    if(state->heapinfo2[i].type > 0){
      addr_block2 = ((void*) (((ADDR2UINT(i)) - 1) * BLOCKSIZE + (char*)state->heapbase2));
      real_addr_block2 =  ((void*) (((ADDR2UINT(i)) - 1) * BLOCKSIZE + (char*)((struct mdesc *)state->s_heap)->heapbase));
      for(j=0; j < (size_t) (BLOCKSIZE >> state->heapinfo2[i].type); j++){
        if(i1 == state->heaplimit){
          if(state->heapinfo2[i].busy_frag.frag_size[j] > 0){
            if(state->equals_to2[i][j] == NULL){
              if(XBT_LOG_ISENABLED(mm_diff, xbt_log_priority_debug)){
                addr_frag2 = (void*) ((char *)addr_block2 + (j << state->heapinfo2[i].type));
                real_addr_frag2 = (void*) ((char *)real_addr_block2 + (j << ((struct mdesc *)state->s_heap)->heapinfo[i].type));
                XBT_DEBUG( "Block %zu, Fragment %zu (%p - %p) not found (size used = %zd)", i, j, addr_frag2, real_addr_frag2, state->heapinfo2[i].busy_frag.frag_size[j]);
                //mmalloc_backtrace_fragment_display((void*)heapinfo2, i, j);
              }
              nb_diff2++;
            }
          }
        }
      }
    }
    i++; 
  }

  if(i1 == state->heaplimit)
    XBT_DEBUG("Number of blocks/fragments not found in heap2 : %d", nb_diff2);

  xbt_dynar_free(&previous);
  real_addr_frag1 = NULL, real_addr_block1 = NULL, real_addr_block2 = NULL, real_addr_frag2 = NULL;

  return ((nb_diff1 > 0) || (nb_diff2 > 0));
}

/**
 *
 * @param state
 * @param real_area1     Process address for state 1
 * @param real_area2     Process address for state 2
 * @param area1          Snapshot address for state 1
 * @param area2          Snapshot address for state 2
 * @param snapshot1      Snapshot of state 1
 * @param snapshot2      Snapshot of state 2
 * @param previous
 * @param info
 * @param other_info
 * @param size
 * @param check_ignore
 */
static int compare_heap_area_without_type(struct s_mm_diff *state, void *real_area1, void *real_area2, void *area1, void *area2, mc_snapshot_t snapshot1, mc_snapshot_t snapshot2, xbt_dynar_t previous, mc_object_info_t info, mc_object_info_t other_info, int size, int check_ignore){

  int i = 0;
  void *addr_pointed1, *addr_pointed2;
  int pointer_align, res_compare;
  ssize_t ignore1, ignore2;

  while(i<size){

    if(check_ignore > 0){
      if((ignore1 = heap_comparison_ignore_size(state->to_ignore1, (char *)real_area1 + i)) != -1){
        if((ignore2 = heap_comparison_ignore_size(state->to_ignore2, (char *)real_area2 + i))  == ignore1){
          if(ignore1 == 0){
            check_ignore--;
            return 0;
          }else{
            i = i + ignore2;
            check_ignore--;
            continue;
          }
        }
      }
    }

    if(memcmp(((char *)area1) + i, ((char *)area2) + i, 1) != 0){

      pointer_align = (i / sizeof(void*)) * sizeof(void*);
      addr_pointed1 = *((void **)((char *)area1 + pointer_align));
      addr_pointed2 = *((void **)((char *)area2 + pointer_align));
      
      if(addr_pointed1 > maestro_stack_start && addr_pointed1 < maestro_stack_end && addr_pointed2 > maestro_stack_start && addr_pointed2 < maestro_stack_end){
        i = pointer_align + sizeof(void *);
        continue;
      }else if((addr_pointed1 > state->s_heap) && ((char *)addr_pointed1 < (char *)state->s_heap + STD_HEAP_SIZE)
               && (addr_pointed2 > state->s_heap) && ((char *)addr_pointed2 < (char *)state->s_heap + STD_HEAP_SIZE)){
        res_compare = compare_heap_area(addr_pointed1, addr_pointed2, snapshot1, snapshot2, previous, info, other_info, NULL, 0);
        if(res_compare == 1){
          return res_compare;
        }
        i = pointer_align + sizeof(void *);
        continue;
      }else{
        return 1;
      }
      
    }
    
    i++;

  }

  return 0;
 
}

/**
 *
 * @param state
 * @param real_area1     Process address for state 1
 * @param real_area2     Process address for state 2
 * @param area1          Snapshot address for state 1
 * @param area2          Snapshot address for state 2
 * @param snapshot1      Snapshot of state 1
 * @param snapshot2      Snapshot of state 2
 * @param previous
 * @param info
 * @param other_info
 * @param type_id
 * @param area_size      either a byte_size or an elements_count (?)
 * @param check_ignore
 * @param pointer_level
 * @return               0 (same), 1 (different), -1 (unknown)
 */
static int compare_heap_area_with_type(struct s_mm_diff *state, void *real_area1, void *real_area2, void *area1, void *area2,
                                       mc_snapshot_t snapshot1, mc_snapshot_t snapshot2,
                                       xbt_dynar_t previous, mc_object_info_t info, mc_object_info_t other_info, char *type_id,
                                       int area_size, int check_ignore, int pointer_level){

  if(is_stack(real_area1) && is_stack(real_area2))
    return 0;

  ssize_t ignore1, ignore2;

  if((check_ignore > 0) && ((ignore1 = heap_comparison_ignore_size(state->to_ignore1, real_area1)) > 0) && ((ignore2 = heap_comparison_ignore_size(state->to_ignore2, real_area2))  == ignore1)){
    return 0;
  }
  
  dw_type_t type = xbt_dict_get_or_null(info->types, type_id);
  dw_type_t subtype, subsubtype;
  int res, elm_size, i, switch_types = 0;
  unsigned int cursor = 0;
  dw_type_t member;
  void *addr_pointed1, *addr_pointed2;;

  switch(type->type){
  case DW_TAG_unspecified_type:
    return 1;

  case DW_TAG_base_type:
    if(type->name!=NULL && strcmp(type->name, "char") == 0){ /* String, hence random (arbitrary ?) size */
      if(real_area1 == real_area2)
        return -1;
      else
        return (memcmp(area1, area2, area_size) != 0);
    }else{
      if(area_size != -1 && type->byte_size != area_size)
        return -1;
      else{
        return  (memcmp(area1, area2, type->byte_size) != 0);
      }
    }
    break;
  case DW_TAG_enumeration_type:
    if(area_size != -1 && type->byte_size != area_size)
      return -1;
    else
      return (memcmp(area1, area2, type->byte_size) != 0);
    break;
  case DW_TAG_typedef:
  case DW_TAG_const_type:
  case DW_TAG_volatile_type:
    return compare_heap_area_with_type(state, real_area1, real_area2, area1, area2, snapshot1, snapshot2, previous, info, other_info, type->dw_type_id, area_size, check_ignore, pointer_level);
    break;
  case DW_TAG_array_type:
    subtype = xbt_dict_get_or_null(info->types, type->dw_type_id);
    switch(subtype->type){
    case DW_TAG_unspecified_type:
      return 1;

    case DW_TAG_base_type:
    case DW_TAG_enumeration_type:
    case DW_TAG_pointer_type:
    case DW_TAG_reference_type:
    case DW_TAG_rvalue_reference_type:
    case DW_TAG_structure_type:
    case DW_TAG_class_type:
    case DW_TAG_union_type:
      if(subtype->byte_size == 0){ /*declaration of the type, need the complete description */
          subtype = xbt_dict_get_or_null(other_info->types_by_name, subtype->name);
          switch_types = 1;
      }
      elm_size = subtype->byte_size;
      break;
    // TODO, just remove the type indirection?
    case DW_TAG_const_type:
    case DW_TAG_typedef:
    case DW_TAG_volatile_type:
      subsubtype = subtype->subtype;
      if(subsubtype->byte_size == 0){ /*declaration of the type, need the complete description */
          subsubtype = xbt_dict_get_or_null(other_info->types_by_name, subtype->name);
          switch_types = 1;
      }
      elm_size = subsubtype->byte_size;
      break;
    default : 
      return 0;
      break;
    }
    for(i=0; i<type->element_count; i++){
      // TODO, add support for variable stride (DW_AT_byte_stride)
      if(switch_types)
        res = compare_heap_area_with_type(state, (char *)real_area1 + (i*elm_size), (char *)real_area2 + (i*elm_size), (char *)area1 + (i*elm_size), (char *)area2 + (i*elm_size), snapshot1, snapshot2, previous, other_info, info, type->dw_type_id, subtype->byte_size, check_ignore, pointer_level);
      else
        res = compare_heap_area_with_type(state, (char *)real_area1 + (i*elm_size), (char *)real_area2 + (i*elm_size), (char *)area1 + (i*elm_size), (char *)area2 + (i*elm_size), snapshot1, snapshot2, previous, info, other_info, type->dw_type_id, subtype->byte_size, check_ignore, pointer_level);
      if(res == 1)
        return res;
    }
    break;
  case DW_TAG_reference_type:
  case DW_TAG_rvalue_reference_type:
  case DW_TAG_pointer_type:
    if(type->dw_type_id && ((dw_type_t)xbt_dict_get_or_null(info->types, type->dw_type_id))->type == DW_TAG_subroutine_type){
      addr_pointed1 = *((void **)(area1)); 
      addr_pointed2 = *((void **)(area2));
      return (addr_pointed1 != addr_pointed2);;
    }else{
      pointer_level++;
      if(pointer_level > 1){ /* Array of pointers */
        for(i=0; i<(area_size/sizeof(void *)); i++){ 
          addr_pointed1 = *((void **)((char *)area1 + (i*sizeof(void *)))); 
          addr_pointed2 = *((void **)((char *)area2 + (i*sizeof(void *)))); 
          if(addr_pointed1 > state->s_heap && (char *)addr_pointed1 < (char*) state->s_heap + STD_HEAP_SIZE && addr_pointed2 > state->s_heap && (char *)addr_pointed2 < (char*) state->s_heap + STD_HEAP_SIZE)
            res =  compare_heap_area(addr_pointed1, addr_pointed2, snapshot1, snapshot2, previous, info, other_info, type->dw_type_id, pointer_level);
          else
            res =  (addr_pointed1 != addr_pointed2);
          if(res == 1)
            return res;
        }
      }else{
        addr_pointed1 = *((void **)(area1)); 
        addr_pointed2 = *((void **)(area2));
        if(addr_pointed1 > state->s_heap && (char *)addr_pointed1 < (char*) state->s_heap + STD_HEAP_SIZE && addr_pointed2 > state->s_heap && (char *)addr_pointed2 < (char*) state->s_heap + STD_HEAP_SIZE)
          return compare_heap_area(addr_pointed1, addr_pointed2, snapshot1, snapshot2, previous, info, other_info, type->dw_type_id, pointer_level);
        else
          return  (addr_pointed1 != addr_pointed2);
      }
    }
    break;
  case DW_TAG_structure_type:
  case DW_TAG_class_type:
    if(type->byte_size == 0){ /*declaration of the structure, need the complete description */
      dw_type_t full_type = xbt_dict_get_or_null(info->types_by_name, type->name);
      if(full_type){
        type = full_type;
      }else{
        type = xbt_dict_get_or_null(other_info->types_by_name, type->name);
        switch_types = 1;
      }
    }
    if(area_size != -1 && type->byte_size != area_size){
      if(area_size>type->byte_size && area_size%type->byte_size == 0){
        for(i=0; i<(area_size/type->byte_size); i++){
          if(switch_types)
            res = compare_heap_area_with_type(state, (char *)real_area1 + (i*type->byte_size), (char *)real_area2 + (i*type->byte_size), (char *)area1 + (i*type->byte_size), (char *)area2 + (i*type->byte_size), snapshot1, snapshot2, previous, other_info, info, type_id, -1, check_ignore, 0);
          else
            res = compare_heap_area_with_type(state, (char *)real_area1 + (i*type->byte_size), (char *)real_area2 + (i*type->byte_size), (char *)area1 + (i*type->byte_size), (char *)area2 + (i*type->byte_size), snapshot1, snapshot2, previous, info, other_info, type_id, -1, check_ignore, 0);
          if(res == 1)
            return res;
        }
      }else{
        return -1;
      }
    }else{
      cursor = 0;
      xbt_dynar_foreach(type->members, cursor, member){
        // TODO, optimize this? (for the offset case)
        char* real_member1 = mc_member_resolve(real_area1, type, member, snapshot1);
        char* real_member2 = mc_member_resolve(real_area2, type, member, snapshot2);
        char* member1 = mc_translate_address((uintptr_t)real_member1, snapshot1);
        char* member2 = mc_translate_address((uintptr_t)real_member2, snapshot2);
        if(switch_types)
          res = compare_heap_area_with_type(state, real_member1, real_member2, member1, member2, snapshot1, snapshot2, previous, other_info, info, member->dw_type_id, -1, check_ignore, 0);
        else
          res = compare_heap_area_with_type(state, real_member1, real_member2, member1, member2, snapshot1, snapshot2, previous, info, other_info, member->dw_type_id, -1, check_ignore, 0);
        if(res == 1){
          return res;
        }
      }
    }
    break;
  case DW_TAG_union_type:
    return compare_heap_area_without_type(state, real_area1, real_area2, area1, area2, snapshot1, snapshot2, previous, info, other_info, type->byte_size, check_ignore);
    break;
  default:
    break;
  }

  return 0;

}

/** Infer the type of a part of the block from the type of the block
 *
 * TODO, handle DW_TAG_array_type as well as arrays of the object ((*p)[5], p[5])
 *
 * TODO, handle subfields ((*p).bar.foo, (*p)[5].bar…)
 *
 * @param  type_id            DWARF type ID of the root address (in info)
 * @param  info               object debug information of the type of ther root address
 * @param  other_info         other debug information
 * @param  area_size
 * @param  switch_type (out)  whether the resulting type is in info (false) or in other_info (true)
 * @return                    DWARF type ID in either info or other_info
 */
static char* get_offset_type(void* real_base_address, char* type_id, int offset, mc_object_info_t info, mc_object_info_t other_info, int area_size, mc_snapshot_t snapshot, int *switch_type){

  // Beginning of the block, the infered variable type if the type of the block:
  if(offset==0)
    return type_id;

  dw_type_t type = xbt_dict_get_or_null(info->types, type_id);
  if(type == NULL){
    type = xbt_dict_get_or_null(other_info->types, type_id);
    *switch_type = 1;
  }

  switch(type->type){
  case DW_TAG_structure_type :
  case DW_TAG_class_type:
    if(type->byte_size == 0){ /*declaration of the structure, need the complete description */
      if(*switch_type == 0){
        dw_type_t full_type = xbt_dict_get_or_null(info->types_by_name, type->name);
        if(full_type){
          type = full_type;
        }else{
          type = xbt_dict_get_or_null(other_info->types_by_name, type->name);
          *switch_type = 1;
        }
      }else{
        dw_type_t full_type = xbt_dict_get_or_null(other_info->types_by_name, type->name);
        if(full_type){
          type = full_type;
        }else{
          type = xbt_dict_get_or_null(info->types_by_name, type->name);
          *switch_type = 0;
        }
      }
    
    }
    if(area_size != -1 && type->byte_size != area_size){
      if(area_size>type->byte_size && area_size%type->byte_size == 0)
        return type_id;
      else
        return NULL;
    }else{
      unsigned int cursor = 0;
      dw_type_t member;
      xbt_dynar_foreach(type->members, cursor, member){ 

        if(!member->location.size) {
          // We have the offset, use it directly (shortcut):
          if(member->offset == offset)
            return member->dw_type_id;
        } else {
          char* real_member = mc_member_resolve(real_base_address, type, member, snapshot);
          if(real_member - (char*)real_base_address == offset)
            return member->dw_type_id;
        }

      }
      return NULL;
    }
    break;
  default:
    /* FIXME : other cases ? */
    return NULL;
    break;
  }
}

/**
 *
 * @param area1          Process address for state 1
 * @param area2          Process address for state 2
 * @param snapshot1      Snapshot of state 1
 * @param snapshot2      Snapshot of state 2
 * @param previous       Pairs of blocks already compared on the current path (or NULL)
 * @param info
 * @param other_info
 * @param type_id        Type of variable
 * @param pointer_level
 * @return 0 (same), 1 (different), -1
 */
int compare_heap_area(void *area1, void* area2, mc_snapshot_t snapshot1, mc_snapshot_t snapshot2, xbt_dynar_t previous, mc_object_info_t info, mc_object_info_t other_info, char *type_id, int pointer_level){

  struct s_mm_diff* state = mm_diff_info;

  int res_compare;
  ssize_t block1, frag1, block2, frag2;
  ssize_t size;
  int check_ignore = 0;

  void *addr_block1, *addr_block2, *addr_frag1, *addr_frag2, *real_addr_block1, *real_addr_block2,  *real_addr_frag1, *real_addr_frag2;
  void *area1_to_compare, *area2_to_compare;
  dw_type_t type = NULL;
  int type_size = -1;
  int offset1 =0, offset2 = 0;
  int new_size1 = -1, new_size2 = -1;
  char *new_type_id1 = NULL, *new_type_id2 = NULL;
  int switch_type = 0;

  int match_pairs = 0;

  if(previous == NULL){
    previous = xbt_dynar_new(sizeof(heap_area_pair_t), heap_area_pair_free_voidp);
    match_pairs = 1;
  }

  // Get block number:
  block1 = ((char*)area1 - (char*)((xbt_mheap_t)state->s_heap)->heapbase) / BLOCKSIZE + 1;
  block2 = ((char*)area2 - (char*)((xbt_mheap_t)state->s_heap)->heapbase) / BLOCKSIZE + 1;

  // If either block is a stack block:
  if(is_block_stack((int)block1) && is_block_stack((int)block2)){
    add_heap_area_pair(previous, block1, -1, block2, -1);
    if(match_pairs){
      match_equals(state, previous);
      xbt_dynar_free(&previous);
    }
    return 0;
  }

  // If either block is not in the expected area of memory:
  if(((char *)area1 < (char*)((xbt_mheap_t)state->s_heap)->heapbase)  || (block1 > state->heapsize1) || (block1 < 1)
    || ((char *)area2 < (char*)((xbt_mheap_t)state->s_heap)->heapbase) || (block2 > state->heapsize2) || (block2 < 1)){
    if(match_pairs){
      xbt_dynar_free(&previous);
    }
    return 1;
  }

  // Snapshot address of the block:
  addr_block1 = ((void*) (((ADDR2UINT(block1)) - 1) * BLOCKSIZE + (char*)state->heapbase1));
  addr_block2 = ((void*) (((ADDR2UINT(block2)) - 1) * BLOCKSIZE + (char*)state->heapbase2));

  // Process address of the block:
  real_addr_block1 = ((void*) (((ADDR2UINT(block1)) - 1) * BLOCKSIZE + (char*)((xbt_mheap_t)state->s_heap)->heapbase));
  real_addr_block2 = ((void*) (((ADDR2UINT(block2)) - 1) * BLOCKSIZE + (char*)((xbt_mheap_t)state->s_heap)->heapbase));

  if(type_id){

    // Lookup type:
    type = xbt_dict_get_or_null(info->types, type_id);
    if(type->byte_size == 0){
      if(type->subtype == NULL){
        dw_type_t full_type = xbt_dict_get_or_null(info->types_by_name, type->name);
        if(full_type)
          type = full_type;
        else
          type = xbt_dict_get_or_null(other_info->types_by_name, type->name);
      }else{
        type = type->subtype;
      }
    }

    // Find type_size:
    if((type->type == DW_TAG_pointer_type) || ((type->type == DW_TAG_base_type) && type->name!=NULL && (!strcmp(type->name, "char"))))
      type_size = -1;
    else
      type_size = type->byte_size;

  }
  
  if((state->heapinfo1[block1].type == -1) && (state->heapinfo2[block2].type == -1)){  /* Free block */

    if(match_pairs){
      match_equals(state, previous);
      xbt_dynar_free(&previous);
    }
    return 0;

  }else if((state->heapinfo1[block1].type == 0) && (state->heapinfo2[block2].type == 0)){ /* Complete block */
    
    // TODO, lookup variable type from block type as done for fragmented blocks

    if(state->equals_to1[block1][0] != NULL && state->equals_to2[block2][0] != NULL){
      if(equal_blocks(state, block1, block2)){
        if(match_pairs){
          match_equals(state, previous);
          xbt_dynar_free(&previous);
        }
        return 0;
      }
    }

    if(type_size != -1){
      if(type_size != state->heapinfo1[block1].busy_block.busy_size
        && type_size != state->heapinfo2[block2].busy_block.busy_size
        && type->name!=NULL && !strcmp(type->name, "s_smx_context")){
        if(match_pairs){
          match_equals(state, previous);
          xbt_dynar_free(&previous);
        }
        return -1;
      }
    }

    if(state->heapinfo1[block1].busy_block.size != state->heapinfo2[block2].busy_block.size){
      if(match_pairs){
        xbt_dynar_free(&previous);
      }
      return 1;
    }

    if(state->heapinfo1[block1].busy_block.busy_size != state->heapinfo2[block2].busy_block.busy_size){
      if(match_pairs){
        xbt_dynar_free(&previous);
      }
      return 1;
    }

    if(!add_heap_area_pair(previous, block1, -1, block2, -1)){
      if(match_pairs){
        match_equals(state, previous);
        xbt_dynar_free(&previous);
      }
      return 0;
    }
 
    size = state->heapinfo1[block1].busy_block.busy_size;
    
    // Remember (basic) type inference.
    // The current data structure only allows us to do this for the whole block.
    if (type_id != NULL && area1==real_addr_block1) {
      xbt_free(state->types1[block1][0]);
      state->types1[block1][0] = strdup(type_id);
    }
    if (type_id != NULL && area2==real_addr_block2) {
      xbt_free(state->types2[block2][0]);
      state->types2[block2][0] = strdup(type_id);
    }

    if(size <= 0){
      if(match_pairs){
        match_equals(state, previous);
        xbt_dynar_free(&previous);
      }
      return 0;
    }

    frag1 = -1;
    frag2 = -1;

    area1_to_compare = addr_block1;
    area2_to_compare = addr_block2;

    if((state->heapinfo1[block1].busy_block.ignore > 0) && (state->heapinfo2[block2].busy_block.ignore == state->heapinfo1[block1].busy_block.ignore))
      check_ignore = state->heapinfo1[block1].busy_block.ignore;
      
  }else if((state->heapinfo1[block1].type > 0) && (state->heapinfo2[block2].type > 0)){ /* Fragmented block */

    // Fragment number:
    frag1 = ((uintptr_t) (ADDR2UINT (area1) % (BLOCKSIZE))) >> state->heapinfo1[block1].type;
    frag2 = ((uintptr_t) (ADDR2UINT (area2) % (BLOCKSIZE))) >> state->heapinfo2[block2].type;

    // Snapshot address of the fragment:
    addr_frag1 = (void*) ((char *)addr_block1 + (frag1 << state->heapinfo1[block1].type));
    addr_frag2 = (void*) ((char *)addr_block2 + (frag2 << state->heapinfo2[block2].type));

    // Process address of the fragment:
    real_addr_frag1 = (void*) ((char *)real_addr_block1 + (frag1 << ((xbt_mheap_t)state->s_heap)->heapinfo[block1].type));
    real_addr_frag2 = (void*) ((char *)real_addr_block2 + (frag2 << ((xbt_mheap_t)state->s_heap)->heapinfo[block2].type));

    // Check the size of the fragments against the size of the type:
    if(type_size != -1){
      if(state->heapinfo1[block1].busy_frag.frag_size[frag1] == -1 || state->heapinfo2[block2].busy_frag.frag_size[frag2] == -1){
        if(match_pairs){
          match_equals(state, previous);
          xbt_dynar_free(&previous);
        }
        return -1;
      }
      if(type_size != state->heapinfo1[block1].busy_frag.frag_size[frag1]|| type_size !=  state->heapinfo2[block2].busy_frag.frag_size[frag2]){
        if(match_pairs){
          match_equals(state, previous);
          xbt_dynar_free(&previous);
        }
        return -1;
      }
    }

    // Check if the blocks are already matched together:
    if(state->equals_to1[block1][frag1] != NULL && state->equals_to2[block2][frag2] != NULL){
      if(equal_fragments(state, block1, frag1, block2, frag2)){
        if(match_pairs){
          match_equals(state, previous);
          xbt_dynar_free(&previous);
        }
        return 0;
      }
    }

    // Compare the size of both fragments:
    if(state->heapinfo1[block1].busy_frag.frag_size[frag1] != state->heapinfo2[block2].busy_frag.frag_size[frag2]){
      if(type_size == -1){
         if(match_pairs){
          match_equals(state, previous);
          xbt_dynar_free(&previous);
        }
        return -1;
      }else{
        if(match_pairs){
          xbt_dynar_free(&previous);
        }
        return 1;
      }
    }
      
    // Size of the fragment:
    size = state->heapinfo1[block1].busy_frag.frag_size[frag1];

    // Remember (basic) type inference.
    // The current data structure only allows us to do this for the whole block.
    if(type_id != NULL && area1==real_addr_frag1){
      xbt_free(state->types1[block1][frag1]);
      state->types1[block1][frag1] = strdup(type_id);
    }
    if(type_id != NULL && area2==real_addr_frag2) {
      xbt_free(state->types2[block2][frag2]);
      state->types2[block2][frag2] = strdup(type_id);
    }

    // The type of the variable is already known:
    if(type_id) {
      new_type_id1 = type_id;
      new_type_id2 = type_id;
    }

    // Type inference from the block type.
    // Correctness bug: we do not know from which object the type comes.
    // This code is disabled for this reason.
    // TODO, fix by using the type instead of the type global DWARF offset.
    else if(0 && (state->types1[block1][frag1] != NULL || state->types2[block2][frag2] != NULL)) {

      offset1 = (char *)area1 - (char *)real_addr_frag1;
      offset2 = (char *)area2 - (char *)real_addr_frag2;

      if(state->types1[block1][frag1] != NULL && state->types2[block2][frag2] != NULL){
        new_type_id1 = get_offset_type(real_addr_frag1, state->types1[block1][frag1], offset1, info, other_info, size, snapshot1, &switch_type);
        new_type_id2 = get_offset_type(real_addr_frag2, state->types2[block2][frag2], offset1, info, other_info, size, snapshot2, &switch_type);
      }else if(state->types1[block1][frag1] != NULL){
        new_type_id1 = get_offset_type(real_addr_frag1, state->types1[block1][frag1], offset1, info, other_info, size, snapshot1, &switch_type);
        new_type_id2 = get_offset_type(real_addr_frag2, state->types1[block1][frag1], offset2, info, other_info, size, snapshot2, &switch_type);
      }else if(state->types2[block2][frag2] != NULL){
        new_type_id1 = get_offset_type(real_addr_frag1, state->types2[block2][frag2], offset1, info, other_info, size, snapshot1, &switch_type);
        new_type_id2 = get_offset_type(real_addr_frag2, state->types2[block2][frag2], offset2, info, other_info, size, snapshot2, &switch_type);
      }else{
        if(match_pairs){
          match_equals(state, previous);
          xbt_dynar_free(&previous);
        }
        return -1;
      }   

      if(new_type_id1 !=  NULL && new_type_id2 !=  NULL && !strcmp(new_type_id1, new_type_id2)){
        if(switch_type){
          type = xbt_dict_get_or_null(other_info->types, new_type_id1);
          while(type->byte_size == 0 && type->dw_type_id != NULL)
            type = xbt_dict_get_or_null(other_info->types, type->dw_type_id);
          new_size1 = type->byte_size;
          type = xbt_dict_get_or_null(other_info->types, new_type_id2);
          while(type->byte_size == 0 && type->dw_type_id != NULL)
            type = xbt_dict_get_or_null(other_info->types, type->dw_type_id);
          new_size2 = type->byte_size;
        }else{
          type = xbt_dict_get_or_null(info->types, new_type_id1);
          while(type->byte_size == 0 && type->dw_type_id != NULL)
            type = xbt_dict_get_or_null(info->types, type->dw_type_id);
          new_size1 = type->byte_size;
          type = xbt_dict_get_or_null(info->types, new_type_id2);
          while(type->byte_size == 0 && type->dw_type_id != NULL)
            type = xbt_dict_get_or_null(info->types, type->dw_type_id);
          new_size2 = type->byte_size;
        }
      }else{
        if(match_pairs){
          match_equals(state, previous);
          xbt_dynar_free(&previous);
        }
        return -1;
      }
    }

    area1_to_compare = (char *)addr_frag1 + offset1;
    area2_to_compare = (char *)addr_frag2 + offset2;
    
    if(new_size1 > 0 && new_size1 == new_size2){
      type_id = new_type_id1;
      size = new_size1;
    }

    if(offset1 == 0 && offset2 == 0){
      if(!add_heap_area_pair(previous, block1, frag1, block2, frag2)){
        if(match_pairs){
          match_equals(state, previous);
          xbt_dynar_free(&previous);
        }
        return 0;
      }
    }

    if(size <= 0){
      if(match_pairs){
        match_equals(state, previous);
        xbt_dynar_free(&previous);
      }
      return 0;
    }
      
    if((state->heapinfo1[block1].busy_frag.ignore[frag1] > 0) && ( state->heapinfo2[block2].busy_frag.ignore[frag2] == state->heapinfo1[block1].busy_frag.ignore[frag1]))
      check_ignore = state->heapinfo1[block1].busy_frag.ignore[frag1];
    
  }else{

    if(match_pairs){
      xbt_dynar_free(&previous);
    }
    return 1;

  }
  

  /* Start comparison*/
  if(type_id != NULL){
    if(switch_type)
      res_compare = compare_heap_area_with_type(state, area1, area2, area1_to_compare, area2_to_compare, snapshot1, snapshot2, previous, other_info, info, type_id, size, check_ignore, pointer_level);
    else
      res_compare = compare_heap_area_with_type(state, area1, area2, area1_to_compare, area2_to_compare, snapshot1, snapshot2, previous, info, other_info, type_id, size, check_ignore, pointer_level);
    if(res_compare == 1){
      if(match_pairs)
        xbt_dynar_free(&previous);
      return res_compare;
    }
  }else{
    if(switch_type)
      res_compare = compare_heap_area_without_type(state, area1, area2, area1_to_compare, area2_to_compare, snapshot1, snapshot2, previous, other_info, info, size, check_ignore);
    else
      res_compare = compare_heap_area_without_type(state, area1, area2, area1_to_compare, area2_to_compare, snapshot1, snapshot2, previous, info, other_info, size, check_ignore);
    if(res_compare == 1){
      if(match_pairs)
        xbt_dynar_free(&previous);
      return res_compare;
    }
  }

  if(match_pairs){
    match_equals(state, previous);
    xbt_dynar_free(&previous);
  }

  return 0;
}

/*********************************************** Miscellaneous ***************************************************/
/****************************************************************************************************************/

// Not used:
static int get_pointed_area_size(void *area, int heap){

  struct s_mm_diff *state = mm_diff_info;

  int block, frag;
  malloc_info *heapinfo;

  if(heap == 1)
    heapinfo = state->heapinfo1;
  else
    heapinfo = state->heapinfo2;

  block = ((char*)area - (char*)((xbt_mheap_t)state->s_heap)->heapbase) / BLOCKSIZE + 1;

  if(((char *)area < (char*)((xbt_mheap_t)state->s_heap)->heapbase)  || (block > state->heapsize1) || (block < 1))
    return -1;

  if(heapinfo[block].type == -1){ /* Free block */
    return -1;  
  }else if(heapinfo[block].type == 0){ /* Complete block */
    return (int)heapinfo[block].busy_block.busy_size;
  }else{
    frag = ((uintptr_t) (ADDR2UINT (area) % (BLOCKSIZE))) >> heapinfo[block].type;
    return (int)heapinfo[block].busy_frag.frag_size[frag];
  }

}

// Not used:
char *get_type_description(mc_object_info_t info, char *type_name){

  xbt_dict_cursor_t dict_cursor;
  char *type_origin;
  dw_type_t type;

  xbt_dict_foreach(info->types, dict_cursor, type_origin, type){
    if(type->name && (strcmp(type->name, type_name) == 0) && type->byte_size > 0){
      xbt_dict_cursor_free(&dict_cursor);
      return type_origin;
    }
  }

  xbt_dict_cursor_free(&dict_cursor);
  return NULL;
}


#ifndef max
#define max( a, b ) ( ((a) > (b)) ? (a) : (b) )
#endif

// Not used:
int mmalloc_linear_compare_heap(xbt_mheap_t heap1, xbt_mheap_t heap2){

  struct s_mm_diff *state = mm_diff_info;

  if(heap1 == NULL && heap1 == NULL){
    XBT_DEBUG("Malloc descriptors null");
    return 0;
  }

  if(heap1->heaplimit != heap2->heaplimit){
    XBT_DEBUG("Different limit of valid info table indices");
    return 1;
  }

  /* Heap information */
  state->heaplimit = ((struct mdesc *)heap1)->heaplimit;

  state->s_heap = (char *)mmalloc_get_current_heap() - STD_HEAP_SIZE - getpagesize();

  state->heapbase1 = (char *)heap1 + BLOCKSIZE;
  state->heapbase2 = (char *)heap2 + BLOCKSIZE;

  state->heapinfo1 = (malloc_info *)((char *)heap1 + ((uintptr_t)((char *)heap1->heapinfo - (char *)state->s_heap)));
  state->heapinfo2 = (malloc_info *)((char *)heap2 + ((uintptr_t)((char *)heap2->heapinfo - (char *)state->s_heap)));

  state->heapsize1 = heap1->heapsize;
  state->heapsize2 = heap2->heapsize;

  /* Start comparison */
  size_t i, j, k;
  void *addr_block1, *addr_block2, *addr_frag1, *addr_frag2;

  int distance = 0;

  /* Check busy blocks*/

  i = 1;

  while(i <= state->heaplimit){

    addr_block1 = ((void*) (((ADDR2UINT(i)) - 1) * BLOCKSIZE + (char*)state->heapbase1));
    addr_block2 = ((void*) (((ADDR2UINT(i)) - 1) * BLOCKSIZE + (char*)state->heapbase2));

    if(state->heapinfo1[i].type != state->heapinfo2[i].type){
  
      distance += BLOCKSIZE;
      XBT_DEBUG("Different type of blocks (%zu) : %d - %d -> distance = %d", i, state->heapinfo1[i].type, state->heapinfo2[i].type, distance);
      i++;
    
    }else{

      if(state->heapinfo1[i].type == -1){ /* Free block */
        i++;
        continue;
      }

      if(state->heapinfo1[i].type == 0){ /* Large block */
       
        if(state->heapinfo1[i].busy_block.size != state->heapinfo2[i].busy_block.size){
          distance += BLOCKSIZE * max(state->heapinfo1[i].busy_block.size, state->heapinfo2[i].busy_block.size);
          i += max(state->heapinfo1[i].busy_block.size, state->heapinfo2[i].busy_block.size);
          XBT_DEBUG("Different larger of cluster at block %zu : %zu - %zu -> distance = %d", i, state->heapinfo1[i].busy_block.size, state->heapinfo2[i].busy_block.size, distance);
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
        while(k < state->heapinfo1[i].busy_block.size * BLOCKSIZE){
          if(memcmp((char *)addr_block1 + k, (char *)addr_block2 + k, 1) != 0){
            distance ++;
          }
          k++;
        } 

        i++;

      }else { /* Fragmented block */

        for(j=0; j < (size_t) (BLOCKSIZE >> state->heapinfo1[i].type); j++){

          addr_frag1 = (void*) ((char *)addr_block1 + (j << state->heapinfo1[i].type));
          addr_frag2 = (void*) ((char *)addr_block2 + (j << state->heapinfo2[i].type));

          if(state->heapinfo1[i].busy_frag.frag_size[j] == 0 && state->heapinfo2[i].busy_frag.frag_size[j] == 0){
            continue;
          }
          
          
          /*if(heapinfo1[i].busy_frag.frag_size[j] != heapinfo2[i].busy_frag.frag_size[j]){
            distance += max(heapinfo1[i].busy_frag.frag_size[j], heapinfo2[i].busy_frag.frag_size[j]);
            XBT_DEBUG("Different size used in fragment %zu in block %zu : %d - %d -> distance = %d", j, i, heapinfo1[i].busy_frag.frag_size[j], heapinfo2[i].busy_frag.frag_size[j], distance); 
            continue;
            }*/
   
          k=0;

          //while(k < max(heapinfo1[i].busy_frag.frag_size[j], heapinfo2[i].busy_frag.frag_size[j])){
          while(k < (BLOCKSIZE / (BLOCKSIZE >> state->heapinfo1[i].type))){
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

