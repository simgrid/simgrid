/* Copyright (c) 2008-2012 Da SimGrid Team. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc_private.h"

#include "xbt/mmalloc.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_compare, mc,
                                "Logging specific to mc_compare");

static int data_program_region_compare(void *d1, void *d2, size_t size);
static int data_libsimgrid_region_compare(void *d1, void *d2, size_t size);
static int heap_region_compare(void *d1, void *d2, size_t size);

static int compare_local_variables(xbt_strbuff_t s1, xbt_strbuff_t s2);
static int compare_stack(stack_region_t s1, stack_region_t s2, void *sp1, void *sp2, void *heap1, void *heap2, xbt_dynar_t equals);
static int is_heap_equality(xbt_dynar_t equals, void *a1, void *a2);

static int data_program_region_compare(void *d1, void *d2, size_t size){
  int distance = 0;
  size_t i = 0;
  
  for(i=0; i<size; i++){
    if(memcmp(((char *)d1) + i, ((char *)d2) + i, 1) != 0){
      XBT_DEBUG("Different byte (offset=%zu) (%p - %p) in data program region", i, (char *)d1 + i, (char *)d2 + i);
      distance++;
    }
  }
  
  XBT_DEBUG("Hamming distance between data program regions : %d", distance);

  return distance;
}

static int data_libsimgrid_region_compare(void *d1, void *d2, size_t size){
  int distance = 0;
  size_t i = 0;
  int pointer_align;
  void *addr_pointed1 = NULL, *addr_pointed2 = NULL;

  for(i=0; i<size; i++){
    if(memcmp(((char *)d1) + i, ((char *)d2) + i, 1) != 0){
      pointer_align = (i / sizeof(void*)) * sizeof(void*);
      addr_pointed1 = *((void **)((char *)d1 + pointer_align));
      addr_pointed2 = *((void **)((char *)d2 + pointer_align));
      if((addr_pointed1 > start_plt && addr_pointed1 < end_plt) || (addr_pointed2 > start_plt && addr_pointed2 < end_plt)){
        continue;
      }else{
        XBT_DEBUG("Different byte (offset=%zu) (%p - %p) in data libsimgrid region", i, (char *)d1 + i, (char *)d2 + i);
        XBT_DEBUG("Addresses pointed : %p - %p\n", addr_pointed1, addr_pointed2);
        distance++;
      }
    }
  }
  
  XBT_DEBUG("Hamming distance between data libsimgrid regions : %d", distance); fflush(NULL);
  
  return distance;
}

static int heap_region_compare(void *d1, void *d2, size_t size){
  
  int distance = 0;
  size_t i = 0;
  
  for(i=0; i<size; i++){
    if(memcmp(((char *)d1) + i, ((char *)d2) + i, 1) != 0){
      XBT_DEBUG("Different byte (offset=%zu) (%p - %p) in heap region", i, (char *)d1 + i, (char *)d2 + i);
      distance++;
    }
  }
  
  XBT_DEBUG("Hamming distance between heap regions : %d (total size : %zu)", distance, size);

  return distance;
}

int snapshot_compare(mc_snapshot_t s1, mc_snapshot_t s2){

  int errors = 0, i;
  xbt_dynar_t stacks1 = xbt_dynar_new(sizeof(stack_region_t), NULL);
  xbt_dynar_t stacks2 = xbt_dynar_new(sizeof(stack_region_t), NULL);
  xbt_dynar_t equals = xbt_dynar_new(sizeof(heap_equality_t), NULL);

  void *heap1 = NULL, *heap2 = NULL;
  
  if(s1->num_reg != s2->num_reg){
    XBT_DEBUG("Different num_reg (s1 = %u, s2 = %u)", s1->num_reg, s2->num_reg);
    return 1;
  }

  for(i=0 ; i< s1->num_reg ; i++){
    
    if(s1->regions[i]->type != s2->regions[i]->type){
      XBT_INFO("Different type of region");
      errors++;
    }
    
    switch(s1->regions[i]->type){
    case 0 :
      /* Compare heapregion */
      if(s1->regions[i]->size != s2->regions[i]->size){
        XBT_DEBUG("Different size of heap (s1 = %zu, s2 = %zu)", s1->regions[i]->size, s2->regions[i]->size);
        return 1;
      }
      if(s1->regions[i]->start_addr != s2->regions[i]->start_addr){
        XBT_DEBUG("Different start addr of heap (s1 = %p, s2 = %p)", s1->regions[i]->start_addr, s2->regions[i]->start_addr);
        return 1;
      }
      if(mmalloc_compare_heap((xbt_mheap_t)s1->regions[i]->data, (xbt_mheap_t)s2->regions[i]->data, &stacks1, &stacks2, &equals)){
        XBT_DEBUG("Different heap (mmalloc_compare)");
        return 1; 
      }
      heap1 = s1->regions[i]->data;
      heap2 = s2->regions[i]->data;
      break;
    case 1 :
      /* Compare data libsimgrid region */
      if(s1->regions[i]->size != s2->regions[i]->size){
        XBT_DEBUG("Different size of libsimgrid (data) (s1 = %zu, s2 = %zu)", s1->regions[i]->size, s2->regions[i]->size);
        return 1;
      }
      if(s1->regions[i]->start_addr != s2->regions[i]->start_addr){
        XBT_DEBUG("Different start addr of libsimgrid (data) (s1 = %p, s2 = %p)", s1->regions[i]->start_addr, s2->regions[i]->start_addr);
        return 1;
      }
      if(data_libsimgrid_region_compare(s1->regions[i]->data, s2->regions[i]->data, s1->regions[i]->size) != 0){
        XBT_DEBUG("Different memcmp for data in libsimgrid");
        return 1;
      }
      break;

    case 2 :
      /* Compare data program region */
      if(s1->regions[i]->size != s2->regions[i]->size){
        XBT_DEBUG("Different size of data program (s1 = %zu, s2 = %zu)", s1->regions[i]->size, s2->regions[i]->size);
        return 1;
      }
      if(s1->regions[i]->start_addr != s2->regions[i]->start_addr){
        XBT_DEBUG("Different start addr of data program (s1 = %p, s2 = %p)", s1->regions[i]->start_addr, s2->regions[i]->start_addr);
        return 1;
      }
      if(data_program_region_compare(s1->regions[i]->data, s2->regions[i]->data, s1->regions[i]->size) != 0){
        XBT_DEBUG("Different memcmp for data in program");
        return 1;
      }
      break;
 
    }

  }

  /* Stacks comparison */
  unsigned int cursor = 1;
  stack_region_t stack_region1, stack_region2;
  void *sp1, *sp2;
  int diff = 0;

  while(cursor < xbt_dynar_length(stacks1)){
    XBT_DEBUG("Stack %d", cursor + 1); 
    stack_region1 = (stack_region_t)(xbt_dynar_get_as(stacks1, cursor, stack_region_t));
    stack_region2 = (stack_region_t)(xbt_dynar_get_as(stacks2, cursor, stack_region_t));
    sp1 = ((mc_snapshot_stack_t)xbt_dynar_get_as(s1->stacks, cursor, mc_snapshot_stack_t))->stack_pointer;
    sp2 = ((mc_snapshot_stack_t)xbt_dynar_get_as(s2->stacks, cursor, mc_snapshot_stack_t))->stack_pointer;
    diff += compare_stack(stack_region1, stack_region2, sp1, sp2, heap1, heap2, equals);
    cursor++;
  }

  XBT_DEBUG("Hamming distance between stacks : %d", diff);
  if(diff>0)
    return 1;

  return 0;
  
}

static int is_heap_equality(xbt_dynar_t equals, void *a1, void *a2){

  unsigned int cursor = 0;
  int start = 0;
  int end = xbt_dynar_length(equals) - 1;
  heap_equality_t current_equality;

  while(start <= end){
    cursor = (start + end) / 2;
    current_equality = (heap_equality_t)xbt_dynar_get_as(equals, cursor, heap_equality_t);
    if(current_equality->address1 == a1){
      if(current_equality->address2 == a2)
        return 1;
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

  return 0;

}


static int compare_stack(stack_region_t s1, stack_region_t s2, void *sp1, void *sp2, void *heap1, void *heap2, xbt_dynar_t equals){
  
  int k = 0, nb_diff = 0;
  long size_used1 = (long)s1->size - (long)((char*)sp1 - (char*)s1->address);
  long size_used2 = (long)s2->size - (long)((char*)sp2 - (char*)s2->address);

  int pointer_align;
  void *addr_pointed1 = NULL, *addr_pointed2 = NULL;  

  if(size_used1 == size_used2){
 
    while(k < size_used1){
      if(memcmp((char *)s1->address + s1->size - k, (char *)s2->address + s2->size - k, 1) != 0){
        pointer_align = ((size_used1 - k) / sizeof(void*)) * sizeof(void*);
        addr_pointed1 = *((void **)(((char*)s1->address + (s1->size - size_used1)) + pointer_align));
        addr_pointed2 = *((void **)(((char*)s2->address + (s2->size - size_used2)) + pointer_align));
        if(is_heap_equality(equals, addr_pointed1, addr_pointed2) == 0){
          if((addr_pointed1 > std_heap) && (addr_pointed1 < (void *)((char *)std_heap + STD_HEAP_SIZE)) && (addr_pointed2 > std_heap) && (addr_pointed2 < (void *)((char *)std_heap + STD_HEAP_SIZE))){
            if(is_free_area(addr_pointed1, (xbt_mheap_t)heap1) == 0 || is_free_area(addr_pointed2, (xbt_mheap_t)heap2) == 0){
              XBT_DEBUG("Difference at offset %d (%p - %p)", k, (char *)s1->address + s1->size - k, (char *)s2->address + s2->size - k);
              nb_diff++;
            }
          }else{
            XBT_DEBUG("Difference at offset %d (%p - %p)", k, (char *)s1->address + s1->size - k, (char *)s2->address + s2->size - k);
            nb_diff++;
          } 
        }
      } 
      k++;
    }

  }else{
    XBT_DEBUG("Different size used between stacks");
    return 1;
  }

  return nb_diff;
}

static int compare_local_variables(xbt_strbuff_t s1, xbt_strbuff_t s2){

  if(strcmp(s1->data, s2->data) != 0){
    XBT_DEBUG("%s", s1->data);
    XBT_DEBUG("%s", s2->data);
    return 1;
  }
  
  return 0;
} 
