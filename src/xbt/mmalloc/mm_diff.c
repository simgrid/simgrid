/* mm_diff - Memory snapshooting and comparison                             */

/* Copyright (c) 2008-2012. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex_interface.h" /* internals of backtrace setup */
#include "xbt/str.h"
#include "mc/mc.h"
#include "xbt/mmalloc.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mm_diff, xbt,
                                "Logging specific to mm_diff in mmalloc");

xbt_dynar_t mc_heap_comparison_ignore;
xbt_dynar_t stacks_areas;
void *maestro_stack_start, *maestro_stack_end;

static void heap_area_pair_free(heap_area_pair_t pair);
static void heap_area_pair_free_voidp(void *d);
static int add_heap_area_pair(xbt_dynar_t list, int block1, int fragment1, int block2, int fragment2);
static int is_new_heap_area_pair(xbt_dynar_t list, int block1, int fragment1, int block2, int fragment2);
static heap_area_t new_heap_area(int block, int fragment);

static size_t heap_comparison_ignore_size(xbt_dynar_t list, void *address);
static void add_heap_equality(xbt_dynar_t equals, void *a1, void *a2);
static void remove_heap_equality(xbt_dynar_t equals, int address, void *a);

static int is_stack(void *address);
static int is_block_stack(int block);
static int equal_blocks(int b1, int b2);

void mmalloc_backtrace_block_display(void* heapinfo, int block){

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

void mmalloc_backtrace_fragment_display(void* heapinfo, int block, int frag){

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

void mmalloc_backtrace_display(void *addr){

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


void *s_heap = NULL, *heapbase1 = NULL, *heapbase2 = NULL;
malloc_info *heapinfo1 = NULL, *heapinfo2 = NULL;
size_t heaplimit = 0, heapsize1 = 0, heapsize2 = 0;
xbt_dynar_t to_ignore1 = NULL, to_ignore2 = NULL;

int ignore_done1 = 0, ignore_done2 = 0;

void init_heap_information(xbt_mheap_t heap1, xbt_mheap_t heap2, xbt_dynar_t i1, xbt_dynar_t i2){

  heaplimit = ((struct mdesc *)heap1)->heaplimit;

  s_heap = (char *)mmalloc_get_current_heap() - STD_HEAP_SIZE - getpagesize();

  heapbase1 = (char *)heap1 + BLOCKSIZE;
  heapbase2 = (char *)heap2 + BLOCKSIZE;

  heapinfo1 = (malloc_info *)((char *)heap1 + ((uintptr_t)((char *)heap1->heapinfo - (char *)s_heap)));
  heapinfo2 = (malloc_info *)((char *)heap2 + ((uintptr_t)((char *)heap2->heapinfo - (char *)s_heap)));

  heapsize1 = heap1->heapsize;
  heapsize2 = heap2->heapsize;

  to_ignore1 = i1;
  to_ignore2 = i2;

  if(MC_is_active()){
    MC_ignore_data_bss(&heaplimit, sizeof(heaplimit));
    MC_ignore_data_bss(&s_heap, sizeof(s_heap));
    MC_ignore_data_bss(&heapbase1, sizeof(heapbase1));
    MC_ignore_data_bss(&heapbase2, sizeof(heapbase2));
    MC_ignore_data_bss(&heapinfo1, sizeof(heapinfo1));
    MC_ignore_data_bss(&heapinfo2, sizeof(heapinfo2));
    MC_ignore_data_bss(&heapsize1, sizeof(heapsize1));
    MC_ignore_data_bss(&heapsize2, sizeof(heapsize2));
    MC_ignore_data_bss(&to_ignore1, sizeof(to_ignore1));
    MC_ignore_data_bss(&to_ignore2, sizeof(to_ignore2));
  }
  
}

int mmalloc_compare_heap(xbt_mheap_t heap1, xbt_mheap_t heap2){

  if(heap1 == NULL && heap1 == NULL){
    XBT_DEBUG("Malloc descriptors null");
    return 0;
  }

  if(heap1->heaplimit != heap2->heaplimit){
    XBT_DEBUG("Different limit of valid info table indices");
    return 1;
  }

  /* Start comparison */
  size_t i1, i2, j1, j2, k;
  size_t current_block = -1;    /* avoid "maybe uninitialized" warning */
  size_t current_fragment;
  void *addr_block1, *addr_block2, *addr_frag1, *addr_frag2;
  int nb_diff1 = 0, nb_diff2 = 0;

  xbt_dynar_t previous = xbt_dynar_new(sizeof(heap_area_pair_t), heap_area_pair_free_voidp);

  int equal, res_compare = 0;
  
  /* Check busy blocks*/

  i1 = 1;

  while(i1 <= heaplimit){

    current_block = i1;

    if(heapinfo1[i1].type == -1){ /* Free block */
      i1++;
      continue;
    }

    addr_block1 = ((void*) (((ADDR2UINT(i1)) - 1) * BLOCKSIZE + (char*)((xbt_mheap_t)s_heap)->heapbase));

    if(heapinfo1[i1].type == 0){  /* Large block */
      
      if(is_stack(addr_block1)){
        for(k=0; k < heapinfo1[i1].busy_block.size; k++)
          heapinfo1[i1+k].busy_block.equal_to = new_heap_area(i1, -1);
        for(k=0; k < heapinfo2[i1].busy_block.size; k++)
          heapinfo2[i1+k].busy_block.equal_to = new_heap_area(i1, -1);
        i1 = i1 + heapinfo1[current_block].busy_block.size;
        continue;
      }

      if(heapinfo1[i1].busy_block.equal_to != NULL){
        i1++;
        continue;
      }
    
      i2 = 1;
      equal = 0;
      res_compare = 0;
  
      /* Try first to associate to same block in the other heap */
      if(heapinfo2[current_block].type == heapinfo1[current_block].type){

        if(heapinfo2[current_block].busy_block.equal_to == NULL){  
        
          addr_block2 = ((void*) (((ADDR2UINT(current_block)) - 1) * BLOCKSIZE + (char*)((xbt_mheap_t)s_heap)->heapbase));
        
          res_compare = compare_area(addr_block1, addr_block2, previous);
        
          if(res_compare == 0){
            for(k=1; k < heapinfo2[current_block].busy_block.size; k++)
              heapinfo2[current_block+k].busy_block.equal_to = new_heap_area(i1, -1);
            for(k=1; k < heapinfo1[current_block].busy_block.size; k++)
              heapinfo1[current_block+k].busy_block.equal_to = new_heap_area(i1, -1);
            equal = 1;
            match_equals(previous);
            i1 = i1 + heapinfo1[current_block].busy_block.size;
          }
        
          xbt_dynar_reset(previous);
        
        }
        
      }

      while(i2 <= heaplimit && !equal){

        addr_block2 = ((void*) (((ADDR2UINT(i2)) - 1) * BLOCKSIZE + (char*)((xbt_mheap_t)s_heap)->heapbase));        
           
        if(i2 == current_block){
          i2++;
          continue;
        }

        if(heapinfo2[i2].type != 0){
          i2++;
          continue;
        }

        if(heapinfo2[i2].busy_block.equal_to != NULL){         
          i2++;
          continue;
        }
        
        res_compare = compare_area(addr_block1, addr_block2, previous);
        
        if(res_compare == 0){
          for(k=1; k < heapinfo2[i2].busy_block.size; k++)
            heapinfo2[i2+k].busy_block.equal_to = new_heap_area(i1, -1);
          for(k=1; k < heapinfo1[i1].busy_block.size; k++)
            heapinfo1[i1+k].busy_block.equal_to = new_heap_area(i2, -1);
          equal = 1;
          match_equals(previous);
          i1 = i1 + heapinfo1[i1].busy_block.size;
        }

        xbt_dynar_reset(previous);

        i2++;

      }

      if(!equal){
        XBT_DEBUG("Block %zu not found (size_used = %zu, addr = %p)", i1, heapinfo1[i1].busy_block.busy_size, addr_block1);
        i1 = heaplimit + 1;
        nb_diff1++;
      }
      
    }else{ /* Fragmented block */

      for(j1=0; j1 < (size_t) (BLOCKSIZE >> heapinfo1[i1].type); j1++){

        current_fragment = j1;

        if(heapinfo1[i1].busy_frag.frag_size[j1] == -1) /* Free fragment */
          continue;

        if(heapinfo1[i1].busy_frag.equal_to[j1] != NULL)
          continue;

        addr_frag1 = (void*) ((char *)addr_block1 + (j1 << heapinfo1[i1].type));

        i2 = 1;
        equal = 0;
        
        /* Try first to associate to same fragment in the other heap */
        if(heapinfo2[current_block].type == heapinfo1[current_block].type){

          if(heapinfo2[current_block].busy_frag.equal_to[current_fragment] == NULL){  
          
            addr_block2 = ((void*) (((ADDR2UINT(current_block)) - 1) * BLOCKSIZE + (char*)((xbt_mheap_t)s_heap)->heapbase));
            addr_frag2 = (void*) ((char *)addr_block2 + (current_fragment << ((xbt_mheap_t)s_heap)->heapinfo[current_block].type));

            res_compare = compare_area(addr_frag1, addr_frag2, previous);

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

            if(heapinfo2[i2].busy_frag.equal_to[j2] != NULL)                
              continue;            
                          
            addr_block2 = ((void*) (((ADDR2UINT(i2)) - 1) * BLOCKSIZE + (char*)((xbt_mheap_t)s_heap)->heapbase));
            addr_frag2 = (void*) ((char *)addr_block2 + (j2 << ((xbt_mheap_t)s_heap)->heapinfo[i2].type));

            res_compare = compare_area(addr_frag1, addr_frag2, previous);
            
            if(res_compare == 0){
              equal = 1;
              match_equals(previous);
              xbt_dynar_reset(previous);
              break;
            }

            xbt_dynar_reset(previous);

          }

          i2++;

        }

        if(heapinfo1[i1].busy_frag.equal_to[j1] == NULL){
          XBT_DEBUG("Block %zu, fragment %zu not found (size_used = %d, address = %p)", i1, j1, heapinfo1[i1].busy_frag.frag_size[j1], addr_frag1);
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
 
  while(i<=heaplimit){
    if(heapinfo1[i].type == 0){
      if(current_block == heaplimit){
        if(heapinfo1[i].busy_block.busy_size > 0){
          if(heapinfo1[i].busy_block.equal_to == NULL){
            if(XBT_LOG_ISENABLED(mm_diff, xbt_log_priority_debug)){
              addr_block1 = ((void*) (((ADDR2UINT(i)) - 1) * BLOCKSIZE + (char*)heapbase1));
              XBT_DEBUG("Block %zu (%p) not found (size used = %zu)", i, addr_block1, heapinfo1[i].busy_block.busy_size);
              //mmalloc_backtrace_block_display((void*)heapinfo1, i);
            }
            nb_diff1++;
          }
        }
      }
      xbt_free(heapinfo1[i].busy_block.equal_to);
      heapinfo1[i].busy_block.equal_to = NULL;
    }
    if(heapinfo1[i].type > 0){
      addr_block1 = ((void*) (((ADDR2UINT(i)) - 1) * BLOCKSIZE + (char*)heapbase1));
      for(j=0; j < (size_t) (BLOCKSIZE >> heapinfo1[i].type); j++){
        if(current_block == heaplimit){
          if(heapinfo1[i].busy_frag.frag_size[j] > 0){
            if(heapinfo1[i].busy_frag.equal_to[j] == NULL){
              if(XBT_LOG_ISENABLED(mm_diff, xbt_log_priority_debug)){
                addr_frag1 = (void*) ((char *)addr_block1 + (j << heapinfo1[i].type));
                XBT_DEBUG("Block %zu, Fragment %zu (%p) not found (size used = %d)", i, j, addr_frag1, heapinfo1[i].busy_frag.frag_size[j]);
                //mmalloc_backtrace_fragment_display((void*)heapinfo1, i, j);
              }
              nb_diff1++;
            }
          }
        }
        xbt_free(heapinfo1[i].busy_frag.equal_to[j]);
        heapinfo1[i].busy_frag.equal_to[j] = NULL;
      }
    }
    i++; 
  }

  if(current_block == heaplimit)
    XBT_DEBUG("Number of blocks/fragments not found in heap1 : %d", nb_diff1);

  i = 1;

  while(i<=heaplimit){
    if(heapinfo2[i].type == 0){
      if(current_block == heaplimit){
        if(heapinfo2[i].busy_block.busy_size > 0){
          if(heapinfo2[i].busy_block.equal_to == NULL){
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
      for(j=0; j < (size_t) (BLOCKSIZE >> heapinfo2[i].type); j++){
        if(current_block == heaplimit){
          if(heapinfo2[i].busy_frag.frag_size[j] > 0){
            if(heapinfo2[i].busy_frag.equal_to[j] == NULL){
              if(XBT_LOG_ISENABLED(mm_diff, xbt_log_priority_debug)){
                addr_frag2 = (void*) ((char *)addr_block2 + (j << heapinfo2[i].type));
                XBT_DEBUG( "Block %zu, Fragment %zu (%p) not found (size used = %d)", i, j, addr_frag2, heapinfo2[i].busy_frag.frag_size[j]);
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

  if(current_block == heaplimit)
    XBT_DEBUG("Number of blocks/fragments not found in heap2 : %d", nb_diff2);

  xbt_dynar_free(&previous);

  return ((nb_diff1 > 0) || (nb_diff2 > 0));
}

void reset_heap_information(){

  size_t i = 0, j;

  while(i<=heaplimit){
    if(heapinfo1[i].type == 0){
      xbt_free(heapinfo1[i].busy_block.equal_to);
      heapinfo1[i].busy_block.equal_to = NULL;
    }
    if(heapinfo1[i].type > 0){
      for(j=0; j < (size_t) (BLOCKSIZE >> heapinfo1[i].type); j++){
        xbt_free(heapinfo1[i].busy_frag.equal_to[j]);
        heapinfo1[i].busy_frag.equal_to[j] = NULL;
      }
    }
    i++; 
  }

  i = 0;

  while(i<=heaplimit){
    if(heapinfo2[i].type == 0){
      xbt_free(heapinfo2[i].busy_block.equal_to);
      heapinfo2[i].busy_block.equal_to = NULL;
    }
    if(heapinfo2[i].type > 0){
      for(j=0; j < (size_t) (BLOCKSIZE >> heapinfo2[i].type); j++){
        xbt_free(heapinfo2[i].busy_frag.equal_to[j]);
        heapinfo2[i].busy_frag.equal_to[j] = NULL;
      }
    }
    i++; 
  }

  ignore_done1 = 0, ignore_done2 = 0;
  s_heap = NULL, heapbase1 = NULL, heapbase2 = NULL;
  heapinfo1 = NULL, heapinfo2 = NULL;
  heaplimit = 0, heapsize1 = 0, heapsize2 = 0;
  to_ignore1 = NULL, to_ignore2 = NULL;

}

static heap_area_t new_heap_area(int block, int fragment){
  heap_area_t area = NULL;
  area = xbt_new0(s_heap_area_t, 1);
  area->block = block;
  area->fragment = fragment;
  return area;
}


static size_t heap_comparison_ignore_size(xbt_dynar_t ignore_list, void *address){

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

  return 0;
}


int compare_area(void *area1, void* area2, xbt_dynar_t previous){

  size_t i = 0, pointer_align = 0, ignore1 = 0, ignore2 = 0;
  void *addr_pointed1, *addr_pointed2;
  int res_compare;
  size_t block1, frag1, block2, frag2, size;
  int check_ignore = 0;

  void *addr_block1, *addr_block2, *addr_frag1, *addr_frag2;
  void *area1_to_compare, *area2_to_compare;

  int match_pairs = 0;

  if(previous == NULL){
    previous = xbt_dynar_new(sizeof(heap_area_pair_t), heap_area_pair_free_voidp);
    match_pairs = 1;
  }

  block1 = ((char*)area1 - (char*)((xbt_mheap_t)s_heap)->heapbase) / BLOCKSIZE + 1;
  block2 = ((char*)area2 - (char*)((xbt_mheap_t)s_heap)->heapbase) / BLOCKSIZE + 1;

  if(is_block_stack((int)block1) && is_block_stack((int)block2))
    return 0;

  if(((char *)area1 < (char*)((xbt_mheap_t)s_heap)->heapbase)  || (block1 > heapsize1) || (block1 < 1) || ((char *)area2 < (char*)((xbt_mheap_t)s_heap)->heapbase) || (block2 > heapsize2) || (block2 < 1))
    return 1;

  addr_block1 = ((void*) (((ADDR2UINT(block1)) - 1) * BLOCKSIZE + (char*)heapbase1));
  addr_block2 = ((void*) (((ADDR2UINT(block2)) - 1) * BLOCKSIZE + (char*)heapbase2));
  
  if(heapinfo1[block1].type == heapinfo2[block2].type){
    
    if(heapinfo1[block1].type == -1){
      return 0;
    }else if(heapinfo1[block1].type == 0){
      if(heapinfo1[block1].busy_block.equal_to != NULL){
        if(equal_blocks(block1, block2)){
          return 0;
        }
      }
      if(heapinfo1[block1].busy_block.size != heapinfo2[block2].busy_block.size)
        return 1;
      if(heapinfo1[block1].busy_block.busy_size != heapinfo2[block2].busy_block.busy_size)
        return 1;
      if(!add_heap_area_pair(previous, block1, -1, block2, -1))
        return 0;

      size = heapinfo1[block1].busy_block.busy_size;
      frag1 = -1;
      frag2 = -1;

      area1_to_compare = addr_block1;
      area2_to_compare = addr_block2;

      if(heapinfo1[block1].busy_block.ignore == 1 || heapinfo2[block2].busy_block.ignore == 1)
        check_ignore = 1;
    }else{
      frag1 = ((uintptr_t) (ADDR2UINT (area1) % (BLOCKSIZE))) >> heapinfo1[block1].type;
      frag2 = ((uintptr_t) (ADDR2UINT (area2) % (BLOCKSIZE))) >> heapinfo2[block2].type;

      if(heapinfo1[block1].busy_frag.frag_size[frag1] != heapinfo2[block2].busy_frag.frag_size[frag2])
        return 1;  
      if(!add_heap_area_pair(previous, block1, frag1, block2, frag2))
        return 0;

      addr_frag1 = (void*) ((char *)addr_block1 + (frag1 << heapinfo1[block1].type));
      addr_frag2 = (void*) ((char *)addr_block2 + (frag2 << heapinfo2[block2].type));

      area1_to_compare = addr_frag1;
      area2_to_compare = addr_frag2;
      
      size = heapinfo1[block1].busy_frag.frag_size[frag1];

      if(size == 0)
        return 0;
      
      if(heapinfo1[block1].busy_frag.ignore[frag1] == 1 || heapinfo2[block2].busy_frag.ignore[frag2] == 1)
        check_ignore = 1;
    }
  }else if((heapinfo1[block1].type > 0) && (heapinfo2[block2].type > 0)){
    frag1 = ((uintptr_t) (ADDR2UINT (area1) % (BLOCKSIZE))) >> heapinfo1[block1].type;
    frag2 = ((uintptr_t) (ADDR2UINT (area2) % (BLOCKSIZE))) >> heapinfo2[block2].type;

    if(heapinfo1[block1].busy_frag.frag_size[frag1] != heapinfo2[block2].busy_frag.frag_size[frag2])
      return 1;       
    if(!add_heap_area_pair(previous, block1, frag1, block2, frag2))
      return 0;

    addr_frag1 = (void*) ((char *)addr_block1 + (frag1 << heapinfo1[block1].type));
    addr_frag2 = (void*) ((char *)addr_block2 + (frag2 << heapinfo2[block2].type));

    area1_to_compare = addr_frag1;
    area2_to_compare = addr_frag2;
      
    size = heapinfo1[block1].busy_frag.frag_size[frag1];

    if(size == 0)
      return 0;

    if(heapinfo1[block1].busy_frag.ignore[frag1] == 1 || heapinfo2[block2].busy_frag.ignore[frag2] == 1)
      check_ignore = 1;   
  }else{
    return 1;
  }
  
  while(i<size){

    if(check_ignore){
      if((ignore_done1 < xbt_dynar_length(to_ignore1)) && ((ignore1 = heap_comparison_ignore_size(to_ignore1, (char *)area1 + i)) > 0)){
        if((ignore_done2 < xbt_dynar_length(to_ignore2)) && ((ignore2 = heap_comparison_ignore_size(to_ignore2, (char *)area2 + i))  == ignore1)){
          i = i + ignore2;
          ignore_done1++;
          ignore_done2++;
          continue;
        }
      }
    }
   
    if(memcmp(((char *)area1_to_compare) + i, ((char *)area2_to_compare) + i, 1) != 0){

      /* Check pointer difference */
      pointer_align = (i / sizeof(void*)) * sizeof(void*);
      addr_pointed1 = *((void **)((char *)area1_to_compare + pointer_align));
      addr_pointed2 = *((void **)((char *)area2_to_compare + pointer_align));
      
      if(addr_pointed1 > maestro_stack_start && addr_pointed1 < maestro_stack_end && addr_pointed2 > maestro_stack_start && addr_pointed2 < maestro_stack_end){
        i = pointer_align + sizeof(void *);
        continue;
      }

      res_compare = compare_area(addr_pointed1, addr_pointed2, previous);
      
      if(res_compare == 1)
        return 1; 
      
      i = pointer_align + sizeof(void *);
      
    }else{

      i++;

    }
  }

  if(match_pairs)
    match_equals(previous);

  return 0;
  

}

static void heap_area_pair_free(heap_area_pair_t pair){
  if (pair){
    xbt_free(pair);
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

void match_equals(xbt_dynar_t list){

  unsigned int cursor = 0;
  heap_area_pair_t current_pair;
  heap_area_t previous_area;

  xbt_dynar_foreach(list, cursor, current_pair){

    if(current_pair->fragment1 != -1){
      
      if(heapinfo1[current_pair->block1].busy_frag.equal_to[current_pair->fragment1] != NULL){    
        previous_area = heapinfo1[current_pair->block1].busy_frag.equal_to[current_pair->fragment1];
        xbt_free(heapinfo2[previous_area->block].busy_frag.equal_to[previous_area->fragment]);
        heapinfo2[previous_area->block].busy_frag.equal_to[previous_area->fragment] = NULL;
        xbt_free(previous_area); 
      }
      if(heapinfo2[current_pair->block2].busy_frag.equal_to[current_pair->fragment2] != NULL){        
        previous_area = heapinfo2[current_pair->block2].busy_frag.equal_to[current_pair->fragment2];
        xbt_free(heapinfo1[previous_area->block].busy_frag.equal_to[previous_area->fragment]);
        heapinfo1[previous_area->block].busy_frag.equal_to[previous_area->fragment] = NULL;
        xbt_free(previous_area);
      }

      heapinfo1[current_pair->block1].busy_frag.equal_to[current_pair->fragment1] = new_heap_area(current_pair->block2, current_pair->fragment2);
      heapinfo2[current_pair->block2].busy_frag.equal_to[current_pair->fragment2] = new_heap_area(current_pair->block1, current_pair->fragment1);

    }else{

      if(heapinfo1[current_pair->block1].busy_block.equal_to != NULL){
        previous_area = heapinfo1[current_pair->block1].busy_block.equal_to;
        xbt_free(heapinfo2[previous_area->block].busy_block.equal_to);
        heapinfo2[previous_area->block].busy_block.equal_to = NULL; 
        xbt_free(previous_area);
      }
      if(heapinfo2[current_pair->block2].busy_block.equal_to != NULL){
        previous_area = heapinfo2[current_pair->block2].busy_block.equal_to;
        xbt_free(heapinfo1[previous_area->block].busy_block.equal_to);
        heapinfo1[previous_area->block].busy_block.equal_to = NULL;
        xbt_free(previous_area);
      }

      heapinfo1[current_pair->block1].busy_block.equal_to = new_heap_area(current_pair->block2, current_pair->fragment2);
      heapinfo2[current_pair->block2].busy_block.equal_to = new_heap_area(current_pair->block1, current_pair->fragment1);

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

static void add_heap_equality(xbt_dynar_t equals, void *a1, void *a2){
  
  if(xbt_dynar_is_empty(equals)){

    heap_equality_t he = xbt_new0(s_heap_equality_t, 1);
    he->address1 = a1;
    he->address2 = a2;

    xbt_dynar_insert_at(equals, 0, &he);
  
  }else{

    unsigned int cursor = 0;
    int start = 0;
    int end = xbt_dynar_length(equals) - 1;
    heap_equality_t current_equality = NULL;

    while(start <= end){
      cursor = (start + end) / 2;
      current_equality = (heap_equality_t)xbt_dynar_get_as(equals, cursor, heap_equality_t);
      if(current_equality->address1 == a1){
        if(current_equality->address2 == a2)
          return;
        if(current_equality->address2 < a2)
          start = cursor + 1;
        if(current_equality->address2 > a2)
          end = cursor - 1;
      }
      if(current_equality->address1 < a1)
        start = cursor + 1;
      if(current_equality->address1 > a1)
        end = cursor - 1; 
    }

    heap_equality_t he = xbt_new0(s_heap_equality_t, 1);
    he->address1 = a1;
    he->address2 = a2;
  
    if(current_equality->address1 < a1)
      xbt_dynar_insert_at(equals, cursor + 1 , &he);
    else
       xbt_dynar_insert_at(equals, cursor, &he); 

  }

}

static void remove_heap_equality(xbt_dynar_t equals, int address, void *a){
  
  unsigned int cursor = 0;
  heap_equality_t current_equality;
  int found = 0;

  if(address == 1){

    int start = 0;
    int end = xbt_dynar_length(equals) - 1;


    while(start <= end && found == 0){
      cursor = (start + end) / 2;
      current_equality = (heap_equality_t)xbt_dynar_get_as(equals, cursor, heap_equality_t);
      if(current_equality->address1 == a)
        found = 1;
      if(current_equality->address1 < a)
        start = cursor + 1;
      if(current_equality->address1 > a)
        end = cursor - 1; 
    }

    if(found == 1)
      xbt_dynar_remove_at(equals, cursor, NULL);
  
  }else{

    xbt_dynar_foreach(equals, cursor, current_equality){
      if(current_equality->address2 == a){
        found = 1;
        break;
      }
    }

    if(found == 1)
      xbt_dynar_remove_at(equals, cursor, NULL);

  }
  
}

int is_free_area(void *area, xbt_mheap_t heap){

  void *sheap = (char *)mmalloc_get_current_heap() - STD_HEAP_SIZE - getpagesize();
  malloc_info *heapinfo = (malloc_info *)((char *)heap + ((uintptr_t)((char *)heap->heapinfo - (char *)sheap)));
  size_t heapsize = heap->heapsize;

  /* Get block number */ 
  size_t block = ((char*)area - (char*)((xbt_mheap_t)sheap)->heapbase) / BLOCKSIZE + 1;
  size_t fragment;

  /* Check if valid block number */
  if((char *)area < (char*)((xbt_mheap_t)sheap)->heapbase || block > heapsize || block < 1)
    return 0;

  if(heapinfo[block].type < 0)
    return 1;

  if(heapinfo[block].type == 0)
    return 0;

  if(heapinfo[block].type > 0){
    fragment = ((uintptr_t) (ADDR2UINT(area) % (BLOCKSIZE))) >> heapinfo[block].type;
    if(heapinfo[block].busy_frag.frag_size[fragment] == 0)
      return 1;  
  }

  return 0;
 
}

static int equal_blocks(b1, b2){
  if(heapinfo1[b1].busy_block.equal_to != NULL){
    if(heapinfo2[b2].busy_block.equal_to != NULL){
      if(((heap_area_t)(heapinfo1[b1].busy_block.equal_to))->block == b2 && ((heap_area_t)(heapinfo2[b2].busy_block.equal_to))->block == b1)
        return 1;
    }
  }
  return 0;
}
