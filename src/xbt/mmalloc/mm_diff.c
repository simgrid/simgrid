/* mm_diff - Memory snapshooting and comparison                             */

/* Copyright (c) 2008-2013. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex_interface.h" /* internals of backtrace setup */
#include "xbt/str.h"
#include "mc/mc.h"
#include "xbt/mmalloc.h"
#include "mc/datatypes.h"

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

__thread void *s_heap = NULL, *heapbase1 = NULL, *heapbase2 = NULL;
__thread malloc_info *heapinfo1 = NULL, *heapinfo2 = NULL;
__thread size_t heaplimit = 0, heapsize1 = 0, heapsize2 = 0;
__thread xbt_dynar_t to_ignore1 = NULL, to_ignore2 = NULL;
__thread heap_area_t **equals_to1, **equals_to2;
__thread type_name **types1, **types2;

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

static void match_equals(xbt_dynar_t list){

  unsigned int cursor = 0;
  heap_area_pair_t current_pair;
  heap_area_t previous_area;

  xbt_dynar_foreach(list, cursor, current_pair){

    if(current_pair->fragment1 != -1){

      if(equals_to1[current_pair->block1][current_pair->fragment1] != NULL){
        previous_area = equals_to1[current_pair->block1][current_pair->fragment1];
        heap_area_free(equals_to2[previous_area->block][previous_area->fragment]);
        equals_to2[previous_area->block][previous_area->fragment] = NULL;
        heap_area_free(previous_area);
      }
      if(equals_to2[current_pair->block2][current_pair->fragment2] != NULL){
        previous_area = equals_to2[current_pair->block2][current_pair->fragment2];
        heap_area_free(equals_to1[previous_area->block][previous_area->fragment]);
        equals_to1[previous_area->block][previous_area->fragment] = NULL;
        heap_area_free(previous_area);
      }

      equals_to1[current_pair->block1][current_pair->fragment1] = new_heap_area(current_pair->block2, current_pair->fragment2);
      equals_to2[current_pair->block2][current_pair->fragment2] = new_heap_area(current_pair->block1, current_pair->fragment1);
      
    }else{

      if(equals_to1[current_pair->block1][0] != NULL){
        previous_area = equals_to1[current_pair->block1][0];
        heap_area_free(equals_to2[previous_area->block][0]);
        equals_to2[previous_area->block][0] = NULL;
        heap_area_free(previous_area);
      }
      if(equals_to2[current_pair->block2][0] != NULL){
        previous_area = equals_to2[current_pair->block2][0];
        heap_area_free(equals_to1[previous_area->block][0]);
        equals_to1[previous_area->block][0] = NULL;
        heap_area_free(previous_area);
      }

      equals_to1[current_pair->block1][0] = new_heap_area(current_pair->block2, current_pair->fragment2);
      equals_to2[current_pair->block2][0] = new_heap_area(current_pair->block1, current_pair->fragment1);

    }

  }
}

static int equal_blocks(int b1, int b2){
  
  if(equals_to1[b1][0]->block == b2 && equals_to2[b2][0]->block == b1)
    return 1;

  return 0;
}

static int equal_fragments(int b1, int f1, int b2, int f2){
  
  if(equals_to1[b1][f1]->block == b2 && equals_to1[b1][f1]->fragment == f2 && equals_to2[b2][f2]->block == b1 && equals_to2[b2][f2]->fragment == f1)
    return 1;

  return 0;
}

int init_heap_information(xbt_mheap_t heap1, xbt_mheap_t heap2, xbt_dynar_t i1, xbt_dynar_t i2){

  if((((struct mdesc *)heap1)->heaplimit != ((struct mdesc *)heap2)->heaplimit) || ((((struct mdesc *)heap1)->heapsize != ((struct mdesc *)heap2)->heapsize) ))
    return -1;

  int i, j;

  heaplimit = ((struct mdesc *)heap1)->heaplimit;

  s_heap = (char *)mmalloc_get_current_heap() - STD_HEAP_SIZE - getpagesize();

  heapbase1 = (char *)heap1 + BLOCKSIZE;
  heapbase2 = (char *)heap2 + BLOCKSIZE;

  heapinfo1 = (malloc_info *)((char *)heap1 + ((uintptr_t)((char *)((struct mdesc *)heap1)->heapinfo - (char *)s_heap)));
  heapinfo2 = (malloc_info *)((char *)heap2 + ((uintptr_t)((char *)((struct mdesc *)heap2)->heapinfo - (char *)s_heap)));

  heapsize1 = heap1->heapsize;
  heapsize2 = heap2->heapsize;

  to_ignore1 = i1;
  to_ignore2 = i2;

  equals_to1 = malloc(heaplimit * sizeof(heap_area_t *));
  types1 = malloc(heaplimit * sizeof(type_name *));
  for(i=0; i<=heaplimit; i++){
    equals_to1[i] = malloc(MAX_FRAGMENT_PER_BLOCK * sizeof(heap_area_t));
    types1[i] = malloc(MAX_FRAGMENT_PER_BLOCK * sizeof(type_name));
    for(j=0; j<MAX_FRAGMENT_PER_BLOCK; j++){
      equals_to1[i][j] = NULL;
      types1[i][j] = NULL;
    }      
  }

  equals_to2 = malloc(heaplimit * sizeof(heap_area_t *));
  types2 = malloc(heaplimit * sizeof(type_name *));
  for(i=0; i<=heaplimit; i++){
    equals_to2[i] = malloc(MAX_FRAGMENT_PER_BLOCK * sizeof(heap_area_t));
    types2[i] = malloc(MAX_FRAGMENT_PER_BLOCK * sizeof(type_name));
    for(j=0; j<MAX_FRAGMENT_PER_BLOCK; j++){
      equals_to2[i][j] = NULL;
      types2[i][j] = NULL;
    }
  }

  if(MC_is_active()){
    MC_ignore_global_variable("heaplimit");
    MC_ignore_global_variable("s_heap");
    MC_ignore_global_variable("heapbase1");
    MC_ignore_global_variable("heapbase2");
    MC_ignore_global_variable("heapinfo1");
    MC_ignore_global_variable("heapinfo2");
    MC_ignore_global_variable("heapsize1");
    MC_ignore_global_variable("heapsize2");
    MC_ignore_global_variable("to_ignore1");
    MC_ignore_global_variable("to_ignore2");
    MC_ignore_global_variable("equals_to1");
    MC_ignore_global_variable("equals_to2");
    MC_ignore_global_variable("types1");
    MC_ignore_global_variable("types2");
  }

  return 0;

}

void reset_heap_information(){

  size_t i = 0, j;

  for(i=0; i<=heaplimit; i++){
    for(j=0; j<MAX_FRAGMENT_PER_BLOCK;j++){
      heap_area_free(equals_to1[i][j]);
      equals_to1[i][j] = NULL;
      heap_area_free(equals_to2[i][j]);
      equals_to2[i][j] = NULL;
      xbt_free(types1[i][j]);
      types1[i][j] = NULL;
      xbt_free(types2[i][j]);
      types2[i][j] = NULL;
    }
    free(equals_to1[i]);
    free(equals_to2[i]);
    free(types1[i]);
    free(types2[i]);
  }

  free(equals_to1);
  free(equals_to2);
  free(types1);
  free(types2);

  s_heap = NULL, heapbase1 = NULL, heapbase2 = NULL;
  heapinfo1 = NULL, heapinfo2 = NULL;
  heaplimit = 0, heapsize1 = 0, heapsize2 = 0;
  to_ignore1 = NULL, to_ignore2 = NULL;
  equals_to1 = NULL, equals_to2 = NULL;
  types1 = NULL, types2 = NULL;

}

int mmalloc_compare_heap(xbt_mheap_t heap1, xbt_mheap_t heap2, xbt_dict_t all_types, xbt_dict_t other_types){

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

  while(i1 <= heaplimit){

    if(heapinfo1[i1].type == -1){ /* Free block */
      i1++;
      continue;
    }

    addr_block1 = ((void*) (((ADDR2UINT(i1)) - 1) * BLOCKSIZE + (char*)((xbt_mheap_t)s_heap)->heapbase));

    if(heapinfo1[i1].type == 0){  /* Large block */
      
      if(is_stack(addr_block1)){
        for(k=0; k < heapinfo1[i1].busy_block.size; k++)
          equals_to1[i1+k][0] = new_heap_area(i1, -1);
        for(k=0; k < heapinfo2[i1].busy_block.size; k++)
          equals_to2[i1+k][0] = new_heap_area(i1, -1);
        i1 += heapinfo1[i1].busy_block.size;
        continue;
      }

      if(equals_to1[i1][0] != NULL){
        i1++;
        continue;
      }
    
      i2 = 1;
      equal = 0;
      res_compare = 0;
  
      /* Try first to associate to same block in the other heap */
      if(heapinfo2[i1].type == heapinfo1[i1].type){

        if(equals_to2[i1][0] == NULL){

          addr_block2 = ((void*) (((ADDR2UINT(i1)) - 1) * BLOCKSIZE + (char*)((xbt_mheap_t)s_heap)->heapbase));
        
          res_compare = compare_heap_area(addr_block1, addr_block2, NULL, all_types, other_types, NULL, 0);
        
          if(res_compare != 1){
            for(k=1; k < heapinfo2[i1].busy_block.size; k++)
              equals_to2[i1+k][0] = new_heap_area(i1, -1);
            for(k=1; k < heapinfo1[i1].busy_block.size; k++)
              equals_to1[i1+k][0] = new_heap_area(i1, -1);
            equal = 1;
            i1 += heapinfo1[i1].busy_block.size;
          }
        
          xbt_dynar_reset(previous);
        
        }
        
      }

      while(i2 <= heaplimit && !equal){

        addr_block2 = ((void*) (((ADDR2UINT(i2)) - 1) * BLOCKSIZE + (char*)((xbt_mheap_t)s_heap)->heapbase));        
           
        if(i2 == i1){
          i2++;
          continue;
        }

        if(heapinfo2[i2].type != 0){
          i2++;
          continue;
        }
    
        if(equals_to2[i2][0] != NULL){  
          i2++;
          continue;
        }
          
        res_compare = compare_heap_area(addr_block1, addr_block2, NULL, all_types, other_types, NULL, 0);
        
        if(res_compare != 1 ){
          for(k=1; k < heapinfo2[i2].busy_block.size; k++)
            equals_to2[i2+k][0] = new_heap_area(i1, -1);
          for(k=1; k < heapinfo1[i1].busy_block.size; k++)
            equals_to1[i1+k][0] = new_heap_area(i2, -1);
          equal = 1;
          i1 += heapinfo1[i1].busy_block.size;
        }

        xbt_dynar_reset(previous);

        i2++;

      }

      if(!equal){
        XBT_DEBUG("Block %zu not found (size_used = %zu, addr = %p)", i1, heapinfo1[i1].busy_block.busy_size, addr_block1);
        i1 = heaplimit + 1;
        nb_diff1++;
          //i1++;
      }
      
    }else{ /* Fragmented block */

      for(j1=0; j1 < (size_t) (BLOCKSIZE >> heapinfo1[i1].type); j1++){

        if(heapinfo1[i1].busy_frag.frag_size[j1] == -1) /* Free fragment */
          continue;

        if(equals_to1[i1][j1] != NULL)
          continue;

        addr_frag1 = (void*) ((char *)addr_block1 + (j1 << heapinfo1[i1].type));

        i2 = 1;
        equal = 0;
        
        /* Try first to associate to same fragment in the other heap */
        if(heapinfo2[i1].type == heapinfo1[i1].type){

          if(equals_to2[i1][j1] == NULL){

            addr_block2 = ((void*) (((ADDR2UINT(i1)) - 1) * BLOCKSIZE + (char*)((xbt_mheap_t)s_heap)->heapbase));
            addr_frag2 = (void*) ((char *)addr_block2 + (j1 << ((xbt_mheap_t)s_heap)->heapinfo[i1].type));

            res_compare = compare_heap_area(addr_frag1, addr_frag2, NULL, all_types, other_types, NULL, 0);

            if(res_compare !=  1)
              equal = 1;
        
            xbt_dynar_reset(previous);

          }

        }

        while(i2 <= heaplimit && !equal){

          if(heapinfo2[i2].type <= 0){
            i2++;
            continue;
          }

          for(j2=0; j2 < (size_t) (BLOCKSIZE >> heapinfo2[i2].type); j2++){

            if(i2 == i1 && j2 == j1)
              continue;
           
            if(equals_to2[i2][j2] != NULL)
              continue;
                          
            addr_block2 = ((void*) (((ADDR2UINT(i2)) - 1) * BLOCKSIZE + (char*)((xbt_mheap_t)s_heap)->heapbase));
            addr_frag2 = (void*) ((char *)addr_block2 + (j2 <<((xbt_mheap_t)s_heap)->heapinfo[i2].type));

            res_compare = compare_heap_area(addr_frag1, addr_frag2, NULL, all_types, other_types, NULL, 0);
            
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
          XBT_DEBUG("Block %zu, fragment %zu not found (size_used = %zd, address = %p)\n", i1, j1, heapinfo1[i1].busy_frag.frag_size[j1], addr_frag1);
          i2 = heaplimit + 1;
          i1 = heaplimit + 1;
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
 
  while(i<=heaplimit){
    if(heapinfo1[i].type == 0){
      if(i1 == heaplimit){
        if(heapinfo1[i].busy_block.busy_size > 0){
          if(equals_to1[i][0] == NULL){            
            if(XBT_LOG_ISENABLED(mm_diff, xbt_log_priority_debug)){
              addr_block1 = ((void*) (((ADDR2UINT(i)) - 1) * BLOCKSIZE + (char*)heapbase1));
              XBT_DEBUG("Block %zu (%p) not found (size used = %zu)", i, addr_block1, heapinfo1[i].busy_block.busy_size);
              //mmalloc_backtrace_block_display((void*)heapinfo1, i);
            }
            nb_diff1++;
          }
        }
      }
    }
    if(heapinfo1[i].type > 0){
      addr_block1 = ((void*) (((ADDR2UINT(i)) - 1) * BLOCKSIZE + (char*)heapbase1));
      real_addr_block1 =  ((void*) (((ADDR2UINT(i)) - 1) * BLOCKSIZE + (char*)((struct mdesc *)s_heap)->heapbase));
      for(j=0; j < (size_t) (BLOCKSIZE >> heapinfo1[i].type); j++){
        if(i1== heaplimit){
          if(heapinfo1[i].busy_frag.frag_size[j] > 0){
            if(equals_to1[i][j] == NULL){
              if(XBT_LOG_ISENABLED(mm_diff, xbt_log_priority_debug)){
                addr_frag1 = (void*) ((char *)addr_block1 + (j << heapinfo1[i].type));
                real_addr_frag1 = (void*) ((char *)real_addr_block1 + (j << ((struct mdesc *)s_heap)->heapinfo[i].type));
                XBT_DEBUG("Block %zu, Fragment %zu (%p - %p) not found (size used = %zd)", i, j, addr_frag1, real_addr_frag1, heapinfo1[i].busy_frag.frag_size[j]);
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

  if(i1 == heaplimit)
    XBT_DEBUG("Number of blocks/fragments not found in heap1 : %d", nb_diff1);

  i = 1;

  while(i<=heaplimit){
    if(heapinfo2[i].type == 0){
      if(i1 == heaplimit){
        if(heapinfo2[i].busy_block.busy_size > 0){
          if(equals_to2[i][0] == NULL){
            if(XBT_LOG_ISENABLED(mm_diff, xbt_log_priority_debug)){
              addr_block2 = ((void*) (((ADDR2UINT(i)) - 1) * BLOCKSIZE + (char*)heapbase2));
              XBT_DEBUG("Block %zu (%p) not found (size used = %zu)", i, addr_block2, heapinfo2[i].busy_block.busy_size);
              //mmalloc_backtrace_block_display((void*)heapinfo2, i);
            }
            nb_diff2++;
          }
        }
      }
    }
    if(heapinfo2[i].type > 0){
      addr_block2 = ((void*) (((ADDR2UINT(i)) - 1) * BLOCKSIZE + (char*)heapbase2));
      real_addr_block2 =  ((void*) (((ADDR2UINT(i)) - 1) * BLOCKSIZE + (char*)((struct mdesc *)s_heap)->heapbase));
      for(j=0; j < (size_t) (BLOCKSIZE >> heapinfo2[i].type); j++){
        if(i1 == heaplimit){
          if(heapinfo2[i].busy_frag.frag_size[j] > 0){
            if(equals_to2[i][j] == NULL){
              if(XBT_LOG_ISENABLED(mm_diff, xbt_log_priority_debug)){
                addr_frag2 = (void*) ((char *)addr_block2 + (j << heapinfo2[i].type));
                real_addr_frag2 = (void*) ((char *)real_addr_block2 + (j << ((struct mdesc *)s_heap)->heapinfo[i].type));
                XBT_DEBUG( "Block %zu, Fragment %zu (%p - %p) not found (size used = %zd)", i, j, addr_frag2, real_addr_frag2, heapinfo2[i].busy_frag.frag_size[j]);
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

  if(i1 == heaplimit)
    XBT_DEBUG("Number of blocks/fragments not found in heap2 : %d", nb_diff2);

  xbt_dynar_free(&previous);
  real_addr_frag1 = NULL, real_addr_block1 = NULL, real_addr_block2 = NULL, real_addr_frag2 = NULL;

  return ((nb_diff1 > 0) || (nb_diff2 > 0));
}

static int compare_heap_area_without_type(void *real_area1, void *real_area2, void *area1, void *area2, xbt_dynar_t previous, xbt_dict_t all_types, xbt_dict_t other_types, int size, int check_ignore){

  int i = 0;
  void *addr_pointed1, *addr_pointed2;
  int pointer_align, ignore1, ignore2, res_compare;

  while(i<size){

    if(check_ignore > 0){
      if((ignore1 = heap_comparison_ignore_size(to_ignore1, (char *)real_area1 + i)) != -1){
        if((ignore2 = heap_comparison_ignore_size(to_ignore2, (char *)real_area2 + i))  == ignore1){
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
      }else if((addr_pointed1 > s_heap) && ((char *)addr_pointed1 < (char *)s_heap + STD_HEAP_SIZE) 
               && (addr_pointed2 > s_heap) && ((char *)addr_pointed2 < (char *)s_heap + STD_HEAP_SIZE)){
        res_compare = compare_heap_area(addr_pointed1, addr_pointed2, previous, all_types, other_types, NULL, 0); 
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


static int compare_heap_area_with_type(void *real_area1, void *real_area2, void *area1, void *area2, 
                                       xbt_dynar_t previous, xbt_dict_t all_types, xbt_dict_t other_types, char *type_id, 
                                       int area_size, int check_ignore, int pointer_level){

  if(is_stack(real_area1) && is_stack(real_area2))
    return 0;

  size_t ignore1, ignore2;

  if((check_ignore > 0) && ((ignore1 = heap_comparison_ignore_size(to_ignore1, real_area1)) > 0) && ((ignore2 = heap_comparison_ignore_size(to_ignore2, real_area2))  == ignore1)){
    return 0;
  }
  
  dw_type_t type = xbt_dict_get_or_null(all_types, type_id);
  dw_type_t subtype, subsubtype;
  int res, elm_size, i, switch_types = 0;
  unsigned int cursor = 0;
  dw_type_t member;
  void *addr_pointed1, *addr_pointed2;;
  char *type_desc;

  switch(type->type){
  case e_dw_base_type:
    if(strcmp(type->name, "char") == 0){ /* String, hence random (arbitrary ?) size */
      if(real_area1 == real_area2)
        return -1;
      else
        return (memcmp(area1, area2, area_size) != 0);
    }else{
      if(area_size != -1 && type->size != area_size)
        return -1;
      else{
        return  (memcmp(area1, area2, type->size) != 0);
      }
    }
    break;
  case e_dw_enumeration_type:
    if(area_size != -1 && type->size != area_size)
      return -1;
    else
      return (memcmp(area1, area2, type->size) != 0);
    break;
  case e_dw_typedef:
    return compare_heap_area_with_type(real_area1, real_area2, area1, area2, previous, all_types, other_types, type->dw_type_id, area_size, check_ignore, pointer_level);
    break;
  case e_dw_const_type:
    return 0;
    break;
  case e_dw_array_type:
    subtype = xbt_dict_get_or_null(all_types, type->dw_type_id);
    switch(subtype->type){
    case e_dw_base_type:
    case e_dw_enumeration_type:
    case e_dw_pointer_type:
    case e_dw_structure_type:
    case e_dw_union_type:
      if(subtype->size == 0){ /*declaration of the type, need the complete description */
        type_desc = get_type_description(all_types, subtype->name);
        if(type_desc){
          subtype = xbt_dict_get_or_null(all_types, type_desc);
        }else{
          subtype = xbt_dict_get_or_null(other_types, get_type_description(other_types, subtype->name));
          switch_types = 1;
        }
      }
      elm_size = subtype->size;
      break;
    case e_dw_typedef:
    case e_dw_volatile_type:
      subsubtype = xbt_dict_get_or_null(all_types, subtype->dw_type_id);
      if(subsubtype->size == 0){ /*declaration of the type, need the complete description */
        type_desc = get_type_description(all_types, subsubtype->name);
        if(type_desc){
          subsubtype = xbt_dict_get_or_null(all_types, type_desc);
        }else{
          subsubtype = xbt_dict_get_or_null(other_types, get_type_description(other_types, subtype->name));
          switch_types = 1;
        }
      }
      elm_size = subsubtype->size;
      break;
    default : 
      return 0;
      break;
    }
    for(i=0; i<type->size; i++){ 
      if(switch_types)
        res = compare_heap_area_with_type((char *)real_area1 + (i*elm_size), (char *)real_area2 + (i*elm_size), (char *)area1 + (i*elm_size), (char *)area2 + (i*elm_size), previous, other_types, all_types, type->dw_type_id, type->size, check_ignore, pointer_level);
      else
        res = compare_heap_area_with_type((char *)real_area1 + (i*elm_size), (char *)real_area2 + (i*elm_size), (char *)area1 + (i*elm_size), (char *)area2 + (i*elm_size), previous, all_types, other_types, type->dw_type_id, type->size, check_ignore, pointer_level);
      if(res == 1)
        return res;
    }
    break;
  case e_dw_pointer_type:
    if(type->dw_type_id && ((dw_type_t)xbt_dict_get_or_null(all_types, type->dw_type_id))->type == e_dw_subroutine_type){
      addr_pointed1 = *((void **)(area1)); 
      addr_pointed2 = *((void **)(area2));
      return (addr_pointed1 != addr_pointed2);;
    }else{
      pointer_level++;
      if(pointer_level > 1){ /* Array of pointers */
        for(i=0; i<(area_size/sizeof(void *)); i++){ 
          addr_pointed1 = *((void **)((char *)area1 + (i*sizeof(void *)))); 
          addr_pointed2 = *((void **)((char *)area2 + (i*sizeof(void *)))); 
          if(addr_pointed1 > s_heap && (char *)addr_pointed1 < (char*) s_heap + STD_HEAP_SIZE && addr_pointed2 > s_heap && (char *)addr_pointed2 < (char*) s_heap + STD_HEAP_SIZE)
            res =  compare_heap_area(addr_pointed1, addr_pointed2, previous, all_types, other_types, type->dw_type_id, pointer_level); 
          else
            res =  (addr_pointed1 != addr_pointed2);
          if(res == 1)
            return res;
        }
      }else{
        addr_pointed1 = *((void **)(area1)); 
        addr_pointed2 = *((void **)(area2));
        if(addr_pointed1 > s_heap && (char *)addr_pointed1 < (char*) s_heap + STD_HEAP_SIZE && addr_pointed2 > s_heap && (char *)addr_pointed2 < (char*) s_heap + STD_HEAP_SIZE)
          return compare_heap_area(addr_pointed1, addr_pointed2, previous, all_types, other_types, type->dw_type_id, pointer_level); 
        else
          return  (addr_pointed1 != addr_pointed2);
      }
    }
    break;
  case e_dw_structure_type:
    if(type->size == 0){ /*declaration of the structure, need the complete description */
      type_desc = get_type_description(all_types, type->name);
      if(type_desc){
        type = xbt_dict_get_or_null(all_types, type_desc);
      }else{
        type = xbt_dict_get_or_null(other_types, get_type_description(other_types, type->name));
        switch_types = 1;
      }
    }
    if(area_size != -1 && type->size != area_size){
      if(area_size>type->size && area_size%type->size == 0){
        for(i=0; i<(area_size/type->size); i++){ 
          if(switch_types)
            res = compare_heap_area_with_type((char *)real_area1 + (i*type->size), (char *)real_area2 + (i*type->size), (char *)area1 + (i*type->size), (char *)area2 + (i*type->size), previous, other_types, all_types, type_id, -1, check_ignore, 0); 
          else
            res = compare_heap_area_with_type((char *)real_area1 + (i*type->size), (char *)real_area2 + (i*type->size), (char *)area1 + (i*type->size), (char *)area2 + (i*type->size), previous, all_types, other_types, type_id, -1, check_ignore, 0); 
          if(res == 1)
            return res;
        }
      }else{
        return -1;
      }
    }else{
      cursor = 0;
      xbt_dynar_foreach(type->members, cursor, member){ 
        if(switch_types)
          res = compare_heap_area_with_type((char *)real_area1 + member->offset, (char *)real_area2 + member->offset, (char *)area1 + member->offset, (char *)area2 + member->offset, previous, other_types, all_types, member->dw_type_id, -1, check_ignore, 0);
        else
          res = compare_heap_area_with_type((char *)real_area1 + member->offset, (char *)real_area2 + member->offset, (char *)area1 + member->offset, (char *)area2 + member->offset, previous, all_types, other_types, member->dw_type_id, -1, check_ignore, 0);  
        if(res == 1){
          return res;
        }
      }
    }
    break;
  case e_dw_union_type:
    return compare_heap_area_without_type(real_area1, real_area2, area1, area2, previous, all_types, other_types, type->size, check_ignore);
    break;
  case e_dw_volatile_type:
    return compare_heap_area_with_type(real_area1, real_area2, area1, area2, previous, all_types, other_types, type->dw_type_id, area_size, check_ignore, pointer_level);
    break;
  default:
    break;
  }

  return 0;

}

static char* get_offset_type(char* type_id, int offset, xbt_dict_t all_types, xbt_dict_t other_types, int area_size, int *switch_type){
  dw_type_t type = xbt_dict_get_or_null(all_types, type_id);
  if(type == NULL){
    type = xbt_dict_get_or_null(other_types, type_id);
    *switch_type = 1;
  }
  char* type_desc;
  switch(type->type){
  case e_dw_structure_type :
    if(type->size == 0){ /*declaration of the structure, need the complete description */
      if(*switch_type == 0){
        type_desc = get_type_description(all_types, type->name);
        if(type_desc){
          type = xbt_dict_get_or_null(all_types, type_desc);
        }else{
          type = xbt_dict_get_or_null(other_types, get_type_description(other_types, type->name));
          *switch_type = 1;
        }
      }else{
        type_desc = get_type_description(other_types, type->name);
        if(type_desc){
          type = xbt_dict_get_or_null(other_types, type_desc);
        }else{
          type = xbt_dict_get_or_null(all_types, get_type_description(other_types, type->name));
          *switch_type = 0;
        }
      }
    
    }
    if(area_size != -1 && type->size != area_size){
      if(area_size>type->size && area_size%type->size == 0)
        return type_id;
      else
        return NULL;
    }else{
      unsigned int cursor = 0;
      dw_type_t member;
      xbt_dynar_foreach(type->members, cursor, member){ 
        if(member->offset == offset)
          return member->dw_type_id;
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

int compare_heap_area(void *area1, void* area2, xbt_dynar_t previous, xbt_dict_t all_types, xbt_dict_t other_types, char *type_id, int pointer_level){

  int res_compare;
  ssize_t block1, frag1, block2, frag2;
  ssize_t size;
  int check_ignore = 0;

  void *addr_block1, *addr_block2, *addr_frag1, *addr_frag2, *real_addr_block1, *real_addr_block2,  *real_addr_frag1, *real_addr_frag2;
  void *area1_to_compare, *area2_to_compare;
  dw_type_t type = NULL;
  char *type_desc;
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

  block1 = ((char*)area1 - (char*)((xbt_mheap_t)s_heap)->heapbase) / BLOCKSIZE + 1;
  block2 = ((char*)area2 - (char*)((xbt_mheap_t)s_heap)->heapbase) / BLOCKSIZE + 1;

  if(is_block_stack((int)block1) && is_block_stack((int)block2)){
    add_heap_area_pair(previous, block1, -1, block2, -1);
    if(match_pairs){
      match_equals(previous);
      xbt_dynar_free(&previous);
    }
    return 0;
  }

  if(((char *)area1 < (char*)((xbt_mheap_t)s_heap)->heapbase)  || (block1 > heapsize1) || (block1 < 1) || ((char *)area2 < (char*)((xbt_mheap_t)s_heap)->heapbase) || (block2 > heapsize2) || (block2 < 1)){
    if(match_pairs){
      xbt_dynar_free(&previous);
    }
    return 1;
  }

  addr_block1 = ((void*) (((ADDR2UINT(block1)) - 1) * BLOCKSIZE + (char*)heapbase1));
  addr_block2 = ((void*) (((ADDR2UINT(block2)) - 1) * BLOCKSIZE + (char*)heapbase2));

  real_addr_block1 = ((void*) (((ADDR2UINT(block1)) - 1) * BLOCKSIZE + (char*)((xbt_mheap_t)s_heap)->heapbase));
  real_addr_block2 = ((void*) (((ADDR2UINT(block2)) - 1) * BLOCKSIZE + (char*)((xbt_mheap_t)s_heap)->heapbase));

  if(type_id){
    type = xbt_dict_get_or_null(all_types, type_id);
    if(type->size == 0){
      if(type->dw_type_id == NULL){
        type_desc = get_type_description(all_types, type->name);
        if(type_desc)
          type = xbt_dict_get_or_null(all_types, type_desc);
        else
          type = xbt_dict_get_or_null(other_types, get_type_description(other_types, type->name));
      }else{
        type = xbt_dict_get_or_null(all_types, type->dw_type_id);
      }
    }
    if((type->type == e_dw_pointer_type) || ((type->type == e_dw_base_type) && (!strcmp(type->name, "char"))))
      type_size = -1;
    else
      type_size = type->size;
  }
  
  if((heapinfo1[block1].type == -1) && (heapinfo2[block2].type == -1)){  /* Free block */

    if(match_pairs){
      match_equals(previous);
      xbt_dynar_free(&previous);
    }
    return 0;

  }else if((heapinfo1[block1].type == 0) && (heapinfo2[block2].type == 0)){ /* Complete block */ 
    
    if(equals_to1[block1][0] != NULL && equals_to2[block2][0] != NULL){
      if(equal_blocks(block1, block2)){
        if(match_pairs){
          match_equals(previous);
          xbt_dynar_free(&previous);
        }
        return 0;
      }
    }

    if(type_size != -1){
      if(type_size != heapinfo1[block1].busy_block.busy_size && type_size != heapinfo2[block2].busy_block.busy_size && !strcmp(type->name, "s_smx_context")){
        if(match_pairs){
          match_equals(previous);
          xbt_dynar_free(&previous);
        }
        return -1;
      }
    }

    if(heapinfo1[block1].busy_block.size != heapinfo2[block2].busy_block.size){
      if(match_pairs){
        xbt_dynar_free(&previous);
      }
      return 1;
    }

    if(heapinfo1[block1].busy_block.busy_size != heapinfo2[block2].busy_block.busy_size){
      if(match_pairs){
        xbt_dynar_free(&previous);
      }
      return 1;
    }

    if(!add_heap_area_pair(previous, block1, -1, block2, -1)){
      if(match_pairs){
        match_equals(previous);
        xbt_dynar_free(&previous);
      }
      return 0;
    }
 
    size = heapinfo1[block1].busy_block.busy_size;
    
    if(type_id != NULL){
      xbt_free(types1[block1][0]);
      xbt_free(types2[block2][0]);
      types1[block1][0] = strdup(type_id);
      types2[block2][0] = strdup(type_id);
    }

    if(size <= 0){
      if(match_pairs){
        match_equals(previous);
        xbt_dynar_free(&previous);
      }
      return 0;
    }

    frag1 = -1;
    frag2 = -1;

    area1_to_compare = addr_block1;
    area2_to_compare = addr_block2;

    if((heapinfo1[block1].busy_block.ignore > 0) && (heapinfo2[block2].busy_block.ignore == heapinfo1[block1].busy_block.ignore))
      check_ignore = heapinfo1[block1].busy_block.ignore;
      
  }else if((heapinfo1[block1].type > 0) && (heapinfo2[block2].type > 0)){ /* Fragmented block */

    frag1 = ((uintptr_t) (ADDR2UINT (area1) % (BLOCKSIZE))) >> heapinfo1[block1].type;
    frag2 = ((uintptr_t) (ADDR2UINT (area2) % (BLOCKSIZE))) >> heapinfo2[block2].type;

    addr_frag1 = (void*) ((char *)addr_block1 + (frag1 << heapinfo1[block1].type));
    addr_frag2 = (void*) ((char *)addr_block2 + (frag2 << heapinfo2[block2].type));

    real_addr_frag1 = (void*) ((char *)real_addr_block1 + (frag1 << ((xbt_mheap_t)s_heap)->heapinfo[block1].type));
    real_addr_frag2 = (void*) ((char *)real_addr_block2 + (frag2 << ((xbt_mheap_t)s_heap)->heapinfo[block2].type));

    if(type_size != -1){
      if(heapinfo1[block1].busy_frag.frag_size[frag1] == -1 || heapinfo2[block2].busy_frag.frag_size[frag2] == -1){
        if(match_pairs){
          match_equals(previous);
          xbt_dynar_free(&previous);
        }
        return -1;
      }
      if(type_size != heapinfo1[block1].busy_frag.frag_size[frag1] || type_size !=  heapinfo2[block2].busy_frag.frag_size[frag2]){
        if(match_pairs){
          match_equals(previous);
          xbt_dynar_free(&previous);
        }
        return -1;
      }
    }

    if(equals_to1[block1][frag1] != NULL && equals_to2[block2][frag2] != NULL){
      if(equal_fragments(block1, frag1, block2, frag2)){
        if(match_pairs){
          match_equals(previous);
          xbt_dynar_free(&previous);
        }
        return 0;
      }
    }

    if(heapinfo1[block1].busy_frag.frag_size[frag1] != heapinfo2[block2].busy_frag.frag_size[frag2]){
      if(type_size == -1){
         if(match_pairs){
          match_equals(previous);
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
      
    size = heapinfo1[block1].busy_frag.frag_size[frag1];

    if(type_id != NULL){
      xbt_free(types1[block1][frag1]);
      xbt_free(types2[block2][frag2]);
      types1[block1][frag1] = strdup(type_id);
      types2[block2][frag2] = strdup(type_id);
    }

    if(real_addr_frag1 != area1 || real_addr_frag2 != area2){
      offset1 = (char *)area1 - (char *)real_addr_frag1;
      offset2 = (char *)area2 - (char *)real_addr_frag2;
      if(types1[block1][frag1] != NULL && types2[block2][frag2] != NULL){
        new_type_id1 = get_offset_type(types1[block1][frag1], offset1, all_types, other_types, size, &switch_type);
        new_type_id2 = get_offset_type(types2[block2][frag2], offset1, all_types, other_types, size, &switch_type);
      }else if(types1[block1][frag1] != NULL){
        new_type_id1 = get_offset_type(types1[block1][frag1], offset1, all_types, other_types, size, &switch_type);
        new_type_id2 = get_offset_type(types1[block1][frag1], offset2, all_types, other_types, size, &switch_type);       
      }else if(types2[block2][frag2] != NULL){
        new_type_id1 = get_offset_type(types2[block2][frag2], offset1, all_types, other_types, size, &switch_type);
        new_type_id2 = get_offset_type(types2[block2][frag2], offset2, all_types, other_types, size, &switch_type);
      }else{
        if(match_pairs){
          match_equals(previous);
          xbt_dynar_free(&previous);
        }
        return -1;
      }   

      if(new_type_id1 !=  NULL && new_type_id2 !=  NULL && !strcmp(new_type_id1, new_type_id2)){
        if(switch_type){
          type = xbt_dict_get_or_null(other_types, new_type_id1);
          while(type->size == 0 && type->dw_type_id != NULL)
            type = xbt_dict_get_or_null(other_types, type->dw_type_id);
          new_size1 = type->size;
          type = xbt_dict_get_or_null(other_types, new_type_id2);
          while(type->size == 0 && type->dw_type_id != NULL)
            type = xbt_dict_get_or_null(other_types, type->dw_type_id);
          new_size2 = type->size;
        }else{
          type = xbt_dict_get_or_null(all_types, new_type_id1);
          while(type->size == 0 && type->dw_type_id != NULL)
            type = xbt_dict_get_or_null(all_types, type->dw_type_id);
          new_size1 = type->size;
          type = xbt_dict_get_or_null(all_types, new_type_id2);
          while(type->size == 0 && type->dw_type_id != NULL)
            type = xbt_dict_get_or_null(all_types, type->dw_type_id);
          new_size2 = type->size;
        }
      }else{
        if(match_pairs){
          match_equals(previous);
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
          match_equals(previous);
          xbt_dynar_free(&previous);
        }
        return 0;
      }
    }

    if(size <= 0){
      if(match_pairs){
        match_equals(previous);
        xbt_dynar_free(&previous);
      }
      return 0;
    }
      
    if((heapinfo1[block1].busy_frag.ignore[frag1] > 0) && ( heapinfo2[block2].busy_frag.ignore[frag2] == heapinfo1[block1].busy_frag.ignore[frag1]))
      check_ignore = heapinfo1[block1].busy_frag.ignore[frag1];
    
  }else{

    if(match_pairs){
      xbt_dynar_free(&previous);
    }
    return 1;

  }
  

  /* Start comparison*/
  if(type_id != NULL){
    if(switch_type)
      res_compare = compare_heap_area_with_type(area1, area2, area1_to_compare, area2_to_compare, previous, other_types, all_types, type_id, size, check_ignore, pointer_level);
    else
      res_compare = compare_heap_area_with_type(area1, area2, area1_to_compare, area2_to_compare, previous, all_types, other_types, type_id, size, check_ignore, pointer_level);
    if(res_compare == 1){
      if(match_pairs)
        xbt_dynar_free(&previous);
      return res_compare;
    }
  }else{
    if(switch_type)
      res_compare = compare_heap_area_without_type(area1, area2, area1_to_compare, area2_to_compare, previous, other_types, all_types, size, check_ignore);
    else
      res_compare = compare_heap_area_without_type(area1, area2, area1_to_compare, area2_to_compare, previous, all_types, other_types, size, check_ignore);
    if(res_compare == 1){
      if(match_pairs)
        xbt_dynar_free(&previous);
      return res_compare;
    }
  }

  if(match_pairs){
    match_equals(previous);
    xbt_dynar_free(&previous);
  }

  return 0;
}

/*********************************************** Miscellaneous ***************************************************/
/****************************************************************************************************************/


int get_pointed_area_size(void *area, int heap){

  int block, frag;
  malloc_info *heapinfo;

  if(heap == 1)
    heapinfo = heapinfo1;
  else
    heapinfo = heapinfo2;

  block = ((char*)area - (char*)((xbt_mheap_t)s_heap)->heapbase) / BLOCKSIZE + 1;

  if(((char *)area < (char*)((xbt_mheap_t)s_heap)->heapbase)  || (block > heapsize1) || (block < 1))
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

char *get_type_description(xbt_dict_t types, char *type_name){

  xbt_dict_cursor_t dict_cursor;
  char *type_origin;
  dw_type_t type;

  xbt_dict_foreach(types, dict_cursor, type_origin, type){
    if(type->name && (strcmp(type->name, type_name) == 0) && type->size > 0){
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

