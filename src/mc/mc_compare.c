/* Copyright (c) 2008-2012 Da SimGrid Team. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc_private.h"

#include "xbt/mmalloc.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_compare, mc,
                                "Logging specific to mc_compare");

static int data_bss_program_region_compare(void *d1, void *d2, size_t size);
static int data_bss_libsimgrid_region_compare(void *d1, void *d2, size_t size);
static int heap_region_compare(void *d1, void *d2, size_t size);

static int compare_stack(stack_region_t s1, stack_region_t s2, void *sp1, void *sp2, void *heap1, void *heap2, xbt_dynar_t equals);
static int is_heap_equality(xbt_dynar_t equals, void *a1, void *a2);
static size_t heap_ignore_size(void *address);
static size_t data_bss_ignore_size(void *address);

static void stack_region_free(stack_region_t s);
static void heap_equality_free(heap_equality_t e);

static int is_stack_ignore_variable(char *frame, char *var_name);
static int compare_local_variables(char *s1, char *s2, xbt_dynar_t heap_equals);

static size_t heap_ignore_size(void *address){
  unsigned int cursor = 0;
  int start = 0;
  int end = xbt_dynar_length(mc_heap_comparison_ignore) - 1;
  mc_heap_ignore_region_t region;

  while(start <= end){
    cursor = (start + end) / 2;
    region = (mc_heap_ignore_region_t)xbt_dynar_get_as(mc_heap_comparison_ignore, cursor, mc_heap_ignore_region_t);
    if(region->address == address)
      return region->size;
    if(region->address < address)
      start = cursor + 1;
    if(region->address > address)
      end = cursor - 1;   
  }

  return 0;
}

static size_t data_bss_ignore_size(void *address){
  unsigned int cursor = 0;
  int start = 0;
  int end = xbt_dynar_length(mc_data_bss_comparison_ignore) - 1;
  mc_data_bss_ignore_variable_t var;

  while(start <= end){
    cursor = (start + end) / 2;
    var = (mc_data_bss_ignore_variable_t)xbt_dynar_get_as(mc_data_bss_comparison_ignore, cursor, mc_data_bss_ignore_variable_t);
    if(var->address == address)
      return var->size;
    if(var->address < address){
      if((void *)((char *)var->address + var->size) > address)
        return (char *)var->address + var->size - (char*)address;
      else
        start = cursor + 1;
    }
    if(var->address > address)
      end = cursor - 1;   
  }

  return 0;
}

static int data_bss_program_region_compare(void *d1, void *d2, size_t size){

  size_t i = 0, ignore_size = 0;
  int pointer_align;
  void *addr_pointed1 = NULL, *addr_pointed2 = NULL;
  
  for(i=0; i<size; i++){
    if(memcmp(((char *)d1) + i, ((char *)d2) + i, 1) != 0){
      if((ignore_size = data_bss_ignore_size((char *)start_data_binary+i)) > 0){
        i = i + ignore_size;
        continue;
      }else if((ignore_size = data_bss_ignore_size((char *)start_bss_binary+i)) > 0){
        i = i + ignore_size;
        continue;
      }
      pointer_align = (i / sizeof(void*)) * sizeof(void*);
      addr_pointed1 = *((void **)((char *)d1 + pointer_align));
      addr_pointed2 = *((void **)((char *)d2 + pointer_align));
      if((addr_pointed1 > start_plt_binary && addr_pointed1 < end_plt_binary) || (addr_pointed2 > start_plt_binary && addr_pointed2 < end_plt_binary)){
        continue;
      }else{
        if(XBT_LOG_ISENABLED(mc_compare, xbt_log_priority_verbose)){
          XBT_VERB("Different byte (offset=%zu) (%p - %p) in data program region", i, (char *)d1 + i, (char *)d2 + i);
          XBT_VERB("Addresses pointed : %p - %p", addr_pointed1, addr_pointed2);
        }
        return 1;
      }
    }
  }
  
  return 0;
}

static int data_bss_libsimgrid_region_compare(void *d1, void *d2, size_t size){

  size_t i = 0, ignore_size = 0;
  int pointer_align;
  void *addr_pointed1 = NULL, *addr_pointed2 = NULL;

  for(i=0; i<size; i++){
    if(memcmp(((char *)d1) + i, ((char *)d2) + i, 1) != 0){
      if((ignore_size = data_bss_ignore_size((char *)start_data_libsimgrid+i)) > 0){
        i = i + ignore_size;
        continue;
      }else if((ignore_size = data_bss_ignore_size((char *)start_bss_libsimgrid+i)) > 0){
        i = i + ignore_size;
        continue;
      }
      pointer_align = (i / sizeof(void*)) * sizeof(void*);
      addr_pointed1 = *((void **)((char *)d1 + pointer_align));
      addr_pointed2 = *((void **)((char *)d2 + pointer_align));
      if((addr_pointed1 > start_plt_libsimgrid && addr_pointed1 < end_plt_libsimgrid) || (addr_pointed2 > start_plt_libsimgrid && addr_pointed2 < end_plt_libsimgrid)){
        continue;
      }else if(addr_pointed1 >= raw_heap && addr_pointed1 <= end_raw_heap && addr_pointed2 >= raw_heap && addr_pointed2 <= end_raw_heap){
        continue;
      }else{
        if(XBT_LOG_ISENABLED(mc_compare, xbt_log_priority_verbose)){
          XBT_VERB("Different byte (offset=%zu) (%p - %p) in libsimgrid region", i, (char *)d1 + i, (char *)d2 + i);
          XBT_VERB("Addresses pointed : %p - %p", addr_pointed1, addr_pointed2);
        }
        return 1;
      }
    }
  }
  
  return 0;
}

static int heap_region_compare(void *d1, void *d2, size_t size){
  
  size_t i = 0;
  
  for(i=0; i<size; i++){
    if(memcmp(((char *)d1) + i, ((char *)d2) + i, 1) != 0){
      if(XBT_LOG_ISENABLED(mc_compare, xbt_log_priority_verbose)){
        XBT_VERB("Different byte (offset=%zu) (%p - %p) in heap region", i, (char *)d1 + i, (char *)d2 + i);
      }
      return 1;
    }
  }
  
  return 0;
}

static void stack_region_free(stack_region_t s){
  if(s){
    xbt_free(s->process_name);
    xbt_free(s);
  }
}

void stack_region_free_voidp(void *s){
  stack_region_free((stack_region_t) * (void **) s);
}

static void heap_equality_free(heap_equality_t e){
  if(e){
    xbt_free(e);
  }
}

void heap_equality_free_voidp(void *e){
  heap_equality_free((heap_equality_t) * (void **) e);
}

int snapshot_compare(mc_snapshot_t s1, mc_snapshot_t s2, mc_comparison_times_t ct1, mc_comparison_times_t ct2){

  int raw_mem = (mmalloc_get_current_heap() == raw_heap);
  
  MC_SET_RAW_MEM;
      
  int errors = 0, i = 0;

  if(s1->num_reg != s2->num_reg){
    if(XBT_LOG_ISENABLED(mc_compare, xbt_log_priority_verbose)){
      XBT_VERB("Different num_reg (s1 = %u, s2 = %u)", s1->num_reg, s2->num_reg);
    }
    if(!raw_mem)
      MC_UNSET_RAW_MEM;
    return 1;
  }

  int heap_index = 0, data_libsimgrid_index = 0, data_program_index = 0;

  /* Get index of regions */
  while(i < s1->num_reg){
    switch(s1->regions[i]->type){
    case 0:
      heap_index = i;
      i++;
      break;
    case 1:
      data_libsimgrid_index = i;
      i++;
      while( i < s1->num_reg && s1->regions[i]->type == 1)
        i++;
      break;
    case 2:
      data_program_index = i;
      i++;
      while( i < s1->num_reg && s1->regions[i]->type == 2)
        i++;
      break;
    }
  }

  if(ct1 != NULL)
    ct1->nb_comparisons++;
  if(ct2 != NULL)
    ct2->nb_comparisons++;

  xbt_os_timer_t global_timer = xbt_os_timer_new();
  xbt_os_timer_t timer = xbt_os_timer_new();

  xbt_os_timer_start(global_timer);

  if(XBT_LOG_ISENABLED(mc_compare, xbt_log_priority_debug))
    xbt_os_timer_start(timer);

  /* Compare number of blocks/fragments used in each heap */
  size_t chunks_used1 = mmalloc_get_chunks_used((xbt_mheap_t)s1->regions[heap_index]->data);
  size_t chunks_used2 = mmalloc_get_chunks_used((xbt_mheap_t)s2->regions[heap_index]->data);
  if(chunks_used1 != chunks_used2){
    if(XBT_LOG_ISENABLED(mc_compare, xbt_log_priority_debug)){
      xbt_os_timer_stop(timer);
      if(ct1 != NULL)
        xbt_dynar_push_as(ct1->chunks_used_comparison_times, double, xbt_os_timer_elapsed(timer));
      if(ct2 != NULL)
        xbt_dynar_push_as(ct2->chunks_used_comparison_times, double, xbt_os_timer_elapsed(timer));
      XBT_DEBUG("Different number of chunks used in each heap : %zu - %zu", chunks_used1, chunks_used2);
      errors++;
    }else{
      if(XBT_LOG_ISENABLED(mc_compare, xbt_log_priority_verbose))
        XBT_VERB("Different number of chunks used in each heap : %zu - %zu", chunks_used1, chunks_used2);
     
      xbt_os_timer_free(timer);
      xbt_os_timer_stop(global_timer);
      if(ct1 != NULL)
        xbt_dynar_push_as(ct1->snapshot_comparison_times, double, xbt_os_timer_elapsed(global_timer));
      if(ct2 != NULL)
        xbt_dynar_push_as(ct2->snapshot_comparison_times, double, xbt_os_timer_elapsed(global_timer));
      xbt_os_timer_free(global_timer);

      if(!raw_mem)
        MC_UNSET_RAW_MEM;

      return 1;
    }
  }else{
    if(XBT_LOG_ISENABLED(mc_compare, xbt_log_priority_debug))
      xbt_os_timer_stop(timer);
  }
  
  if(XBT_LOG_ISENABLED(mc_compare, xbt_log_priority_debug))
    xbt_os_timer_start(timer);

  /* Compare size of stacks */
  unsigned int cursor = 0;
  void *addr_stack1, *addr_stack2;
  void *sp1, *sp2;
  size_t size_used1, size_used2;
  int is_diff = 0;
  while(cursor < xbt_dynar_length(stacks_areas)){
    addr_stack1 = (char *)s1->regions[heap_index]->data + ((char *)((stack_region_t)xbt_dynar_get_as(stacks_areas, cursor, stack_region_t))->address - (char *)std_heap);
    addr_stack2 = (char *)s2->regions[heap_index]->data + ((char *)((stack_region_t)xbt_dynar_get_as(stacks_areas, cursor, stack_region_t))->address - (char *)std_heap);
    sp1 = ((mc_snapshot_stack_t)xbt_dynar_get_as(s1->stacks, cursor, mc_snapshot_stack_t))->stack_pointer;
    sp2 = ((mc_snapshot_stack_t)xbt_dynar_get_as(s2->stacks, cursor, mc_snapshot_stack_t))->stack_pointer;
    size_used1 = ((stack_region_t)xbt_dynar_get_as(stacks_areas, cursor, stack_region_t))->size - ((char*)sp1 - (char*)addr_stack1);
    size_used2 = ((stack_region_t)xbt_dynar_get_as(stacks_areas, cursor, stack_region_t))->size - ((char*)sp2 - (char*)addr_stack2);
    if(size_used1 != size_used2){
      if(XBT_LOG_ISENABLED(mc_compare, xbt_log_priority_debug)){
        if(is_diff == 0){
          xbt_os_timer_stop(timer);
          if(ct1 != NULL)
            xbt_dynar_push_as(ct1->stacks_sizes_comparison_times, double, xbt_os_timer_elapsed(timer));
          if(ct2 != NULL)
            xbt_dynar_push_as(ct2->stacks_sizes_comparison_times, double, xbt_os_timer_elapsed(timer));
        }
        XBT_DEBUG("Different size used in stacks : %zu - %zu", size_used1, size_used2);
        errors++;
        is_diff = 1;
      }else{
        if(XBT_LOG_ISENABLED(mc_compare, xbt_log_priority_verbose))
          XBT_VERB("Different size used in stacks : %zu - %zu", size_used1, size_used2);
 
        xbt_os_timer_free(timer);
        xbt_os_timer_stop(global_timer);
        if(ct1 != NULL)
          xbt_dynar_push_as(ct1->snapshot_comparison_times, double, xbt_os_timer_elapsed(global_timer));
        if(ct2 != NULL)
          xbt_dynar_push_as(ct2->snapshot_comparison_times, double, xbt_os_timer_elapsed(global_timer));
        xbt_os_timer_free(global_timer);

        if(!raw_mem)
          MC_UNSET_RAW_MEM;

        return 1;
      }
    }
    cursor++;
  }

  if(XBT_LOG_ISENABLED(mc_compare, xbt_log_priority_debug)){
    if(is_diff == 0)
      xbt_os_timer_stop(timer);
    xbt_os_timer_start(timer);
  }
  
  /* Compare program data segment(s) */
  is_diff = 0;
  i = data_program_index;
  while(i < s1->num_reg && s1->regions[i]->type == 2){
    if(data_bss_program_region_compare(s1->regions[i]->data, s2->regions[i]->data, s1->regions[i]->size) != 0){
      if(XBT_LOG_ISENABLED(mc_compare, xbt_log_priority_debug)){
        if(is_diff == 0){
          xbt_os_timer_stop(timer);
          if(ct1 != NULL)
            xbt_dynar_push_as(ct1->program_data_segment_comparison_times, double, xbt_os_timer_elapsed(timer));
          if(ct2 != NULL)
           xbt_dynar_push_as(ct2->program_data_segment_comparison_times, double, xbt_os_timer_elapsed(timer));
        }
        XBT_DEBUG("Different memcmp for data in program");
        errors++;
        is_diff = 1;
      }else{
        if(XBT_LOG_ISENABLED(mc_compare, xbt_log_priority_verbose))
          XBT_VERB("Different memcmp for data in program"); 

        xbt_os_timer_free(timer);
        xbt_os_timer_stop(global_timer);
        if(ct1 != NULL)
          xbt_dynar_push_as(ct1->snapshot_comparison_times, double, xbt_os_timer_elapsed(global_timer));
        if(ct2 != NULL)
          xbt_dynar_push_as(ct2->snapshot_comparison_times, double, xbt_os_timer_elapsed(global_timer));
        xbt_os_timer_free(global_timer);

        if(!raw_mem)
          MC_UNSET_RAW_MEM;
        return 1;
      }
    }
    i++;
  }

  if(XBT_LOG_ISENABLED(mc_compare, xbt_log_priority_debug)){
    if(is_diff == 0)
      xbt_os_timer_stop(timer);
    xbt_os_timer_start(timer);
  }

  /* Compare libsimgrid data segment(s) */
  is_diff = 0;
  i = data_libsimgrid_index;
  while(i < s1->num_reg && s1->regions[i]->type == 1){
    if(data_bss_libsimgrid_region_compare(s1->regions[i]->data, s2->regions[i]->data, s1->regions[i]->size) != 0){
      if(XBT_LOG_ISENABLED(mc_compare, xbt_log_priority_debug)){
        if(is_diff == 0){
          xbt_os_timer_stop(timer);
          if(ct1 != NULL)
            xbt_dynar_push_as(ct1->libsimgrid_data_segment_comparison_times, double, xbt_os_timer_elapsed(timer));
          if(ct2 != NULL)
            xbt_dynar_push_as(ct2->libsimgrid_data_segment_comparison_times, double, xbt_os_timer_elapsed(timer));
        }
        XBT_DEBUG("Different memcmp for data in libsimgrid");
        errors++;
        is_diff = 1;
      }else{
        if(XBT_LOG_ISENABLED(mc_compare, xbt_log_priority_verbose))
          XBT_VERB("Different memcmp for data in libsimgrid");
         
        xbt_os_timer_free(timer);
        xbt_os_timer_stop(global_timer);
        if(ct1 != NULL)
          xbt_dynar_push_as(ct1->snapshot_comparison_times, double, xbt_os_timer_elapsed(global_timer));
        if(ct2 != NULL)
          xbt_dynar_push_as(ct2->snapshot_comparison_times, double, xbt_os_timer_elapsed(global_timer));
        xbt_os_timer_free(global_timer);
        
        if(!raw_mem)
          MC_UNSET_RAW_MEM;
        return 1;
      }
    }
    i++;
  }

  if(XBT_LOG_ISENABLED(mc_compare, xbt_log_priority_debug)){
    if(is_diff == 0)
      xbt_os_timer_stop(timer);
    xbt_os_timer_start(timer);
  }

  /* Compare heap */
  xbt_dynar_t stacks1 = xbt_dynar_new(sizeof(stack_region_t), stack_region_free_voidp);
  xbt_dynar_t stacks2 = xbt_dynar_new(sizeof(stack_region_t), stack_region_free_voidp);
  xbt_dynar_t equals = xbt_dynar_new(sizeof(heap_equality_t), heap_equality_free_voidp);
  
  void *heap1 = s1->regions[heap_index]->data, *heap2 = s2->regions[heap_index]->data;
 
  if(mmalloc_compare_heap((xbt_mheap_t)s1->regions[heap_index]->data, (xbt_mheap_t)s2->regions[heap_index]->data, &stacks1, &stacks2, &equals)){

    if(XBT_LOG_ISENABLED(mc_compare, xbt_log_priority_debug)){
      xbt_os_timer_stop(timer);
      if(ct1 != NULL)
        xbt_dynar_push_as(ct1->heap_comparison_times, double, xbt_os_timer_elapsed(timer));
      if(ct2 != NULL)
        xbt_dynar_push_as(ct2->heap_comparison_times, double, xbt_os_timer_elapsed(timer));
      XBT_DEBUG("Different heap (mmalloc_compare)");
      errors++;
    }else{

      xbt_os_timer_free(timer);
      xbt_dynar_free(&stacks1);
      xbt_dynar_free(&stacks2);
      xbt_dynar_free(&equals);
 
      if(XBT_LOG_ISENABLED(mc_compare, xbt_log_priority_verbose))
        XBT_VERB("Different heap (mmalloc_compare)");
       
      xbt_os_timer_stop(global_timer);
      if(ct1 != NULL)
        xbt_dynar_push_as(ct1->snapshot_comparison_times, double, xbt_os_timer_elapsed(global_timer));
      if(ct2 != NULL)
        xbt_dynar_push_as(ct2->snapshot_comparison_times, double, xbt_os_timer_elapsed(global_timer));
      xbt_os_timer_free(global_timer);

      if(!raw_mem)
        MC_UNSET_RAW_MEM;
      return 1;
    } 
  }else{
    if(XBT_LOG_ISENABLED(mc_compare, xbt_log_priority_debug))
      xbt_os_timer_stop(timer);
  }

  if(XBT_LOG_ISENABLED(mc_compare, xbt_log_priority_debug))
    xbt_os_timer_start(timer);

  /* Stacks comparison */
  cursor = 0;
  stack_region_t stack_region1, stack_region2;
  int diff = 0, diff_local = 0;
  is_diff = 0;

  while(cursor < xbt_dynar_length(stacks1)){
    stack_region1 = (stack_region_t)(xbt_dynar_get_as(stacks1, cursor, stack_region_t));
    stack_region2 = (stack_region_t)(xbt_dynar_get_as(stacks2, cursor, stack_region_t));
    sp1 = ((mc_snapshot_stack_t)xbt_dynar_get_as(s1->stacks, cursor, mc_snapshot_stack_t))->stack_pointer;
    sp2 = ((mc_snapshot_stack_t)xbt_dynar_get_as(s2->stacks, cursor, mc_snapshot_stack_t))->stack_pointer;
    diff = compare_stack(stack_region1, stack_region2, sp1, sp2, heap1, heap2, equals);

    if(diff > 0){ /* Differences may be due to padding */  
      diff_local = compare_local_variables(((mc_snapshot_stack_t)xbt_dynar_get_as(s1->stacks, cursor, mc_snapshot_stack_t))->local_variables->data, ((mc_snapshot_stack_t)xbt_dynar_get_as(s2->stacks, cursor, mc_snapshot_stack_t))->local_variables->data, equals);
      if(diff_local > 0){
        if(XBT_LOG_ISENABLED(mc_compare, xbt_log_priority_debug)){
          if(is_diff == 0){
            xbt_os_timer_stop(timer);
            if(ct1 != NULL)
              xbt_dynar_push_as(ct1->stacks_comparison_times, double, xbt_os_timer_elapsed(timer));
            if(ct2 != NULL)
              xbt_dynar_push_as(ct2->stacks_comparison_times, double, xbt_os_timer_elapsed(timer));
          }
          XBT_DEBUG("Different local variables between stacks %d", cursor + 1);
          errors++;
          is_diff = 1;
        }else{
          xbt_dynar_free(&stacks1);
          xbt_dynar_free(&stacks2);
          xbt_dynar_free(&equals);

          if(XBT_LOG_ISENABLED(mc_compare, xbt_log_priority_verbose))
            XBT_VERB("Different local variables between stacks %d", cursor + 1);
          
          xbt_os_timer_free(timer);
          xbt_os_timer_stop(global_timer);
          if(ct1 != NULL)
            xbt_dynar_push_as(ct1->snapshot_comparison_times, double, xbt_os_timer_elapsed(global_timer));
          if(ct2 != NULL)
            xbt_dynar_push_as(ct2->snapshot_comparison_times, double, xbt_os_timer_elapsed(global_timer));
          xbt_os_timer_free(global_timer);
          
          if(!raw_mem)
            MC_UNSET_RAW_MEM;

          return 1;
        }
      }
    }
    cursor++;
  }

  xbt_dynar_free(&stacks1);
  xbt_dynar_free(&stacks2);
  xbt_dynar_free(&equals);

  if(XBT_LOG_ISENABLED(mc_compare, xbt_log_priority_debug))    
    xbt_os_timer_free(timer);

  if(!XBT_LOG_ISENABLED(mc_compare, xbt_log_priority_debug)){
    xbt_os_timer_stop(global_timer);
    if(ct1 != NULL)
      xbt_dynar_push_as(ct1->snapshot_comparison_times, double, xbt_os_timer_elapsed(global_timer));
    if(ct2 != NULL)
      xbt_dynar_push_as(ct2->snapshot_comparison_times, double, xbt_os_timer_elapsed(global_timer));
  }
  xbt_os_timer_free(global_timer);

  if(!raw_mem)
    MC_UNSET_RAW_MEM;

  return errors > 0;
  
}

static int is_stack_ignore_variable(char *frame, char *var_name){

  unsigned int cursor = 0;
  int start = 0;
  int end = xbt_dynar_length(mc_stack_comparison_ignore) - 1;
  mc_stack_ignore_variable_t current_var;

  while(start <= end){
    cursor = (start + end) / 2;
    current_var = (mc_stack_ignore_variable_t)xbt_dynar_get_as(mc_stack_comparison_ignore, cursor, mc_stack_ignore_variable_t);
    if(strcmp(current_var->frame, frame) == 0 || strcmp(current_var->frame, "*") == 0){
      if(strcmp(current_var->var_name, var_name) == 0)
        return 1;
      if(strcmp(current_var->var_name, var_name) < 0)
        start = cursor + 1;
      if(strcmp(current_var->var_name, var_name) > 0)
        end = cursor - 1;
    }else if(strcmp(current_var->frame, frame) < 0){
      start = cursor + 1;
    }else if(strcmp(current_var->frame, frame) > 0){
      end = cursor - 1; 
    }
  }

  return 0;
}

static int compare_local_variables(char *s1, char *s2, xbt_dynar_t heap_equals){
  
  xbt_dynar_t tokens1 = xbt_str_split(s1, NULL);
  xbt_dynar_t tokens2 = xbt_str_split(s2, NULL);

  xbt_dynar_t s_tokens1, s_tokens2;
  unsigned int cursor = 0;
  void *addr1, *addr2;
  char *ip1 = NULL, *ip2 = NULL;
  
  while(cursor < xbt_dynar_length(tokens1)){
    s_tokens1 = xbt_str_split(xbt_dynar_get_as(tokens1, cursor, char *), "=");
    s_tokens2 = xbt_str_split(xbt_dynar_get_as(tokens2, cursor, char *), "=");
    if(xbt_dynar_length(s_tokens1) > 1 && xbt_dynar_length(s_tokens2) > 1){
      if((strcmp(xbt_dynar_get_as(s_tokens1, 0, char *), "ip") == 0) && (strcmp(xbt_dynar_get_as(s_tokens2, 0, char *), "ip") == 0)){
        ip1 = strdup(xbt_dynar_get_as(s_tokens1, 1, char *));
        ip2 = strdup(xbt_dynar_get_as(s_tokens2, 1, char *));
        /*if(XBT_LOG_ISENABLED(mc_compare, xbt_log_priority_verbose))
          XBT_VERB("Instruction pointer : %s, Instruction pointer : %s", ip1, ip2);*/
      }
      if(strcmp(xbt_dynar_get_as(s_tokens1, 1, char *), xbt_dynar_get_as(s_tokens2, 1, char *)) != 0){   
        /* Ignore this variable ?  */
        if(is_stack_ignore_variable(ip1, xbt_dynar_get_as(s_tokens1, 0, char *)) && is_stack_ignore_variable(ip2, xbt_dynar_get_as(s_tokens2, 0, char *))){
          xbt_dynar_free(&s_tokens1);
          xbt_dynar_free(&s_tokens2);
          cursor++;
          continue;
        }
        addr1 = (void *) strtoul(xbt_dynar_get_as(s_tokens1, 1, char *), NULL, 16);
        addr2 = (void *) strtoul(xbt_dynar_get_as(s_tokens2, 1, char *), NULL, 16);
        if(is_heap_equality(heap_equals, addr1, addr2) == 0){
          if(XBT_LOG_ISENABLED(mc_compare, xbt_log_priority_verbose))
            XBT_VERB("Variable %s is different between stacks : %s - %s", xbt_dynar_get_as(s_tokens1, 0, char *), xbt_dynar_get_as(s_tokens1, 1, char *), xbt_dynar_get_as(s_tokens2, 1, char *));
          xbt_dynar_free(&s_tokens1);
          xbt_dynar_free(&s_tokens2);
          return 1;
        }
      }
    }
    xbt_dynar_free(&s_tokens1);
    xbt_dynar_free(&s_tokens2);
    cursor++;
  }

  xbt_dynar_free(&tokens1);
  xbt_dynar_free(&tokens2);
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
  
  size_t k = 0;
  size_t size_used1 = s1->size - ((char*)sp1 - (char*)s1->address);
  size_t size_used2 = s2->size - ((char*)sp2 - (char*)s2->address);

  int pointer_align;
  void *addr_pointed1 = NULL, *addr_pointed2 = NULL;  
  
  while(k < size_used1){
    if(memcmp((char *)s1->address + s1->size - k, (char *)s2->address + s2->size - k, 1) != 0){
      pointer_align = ((size_used1 - k) / sizeof(void*)) * sizeof(void*);
      addr_pointed1 = *((void **)(((char*)s1->address + (s1->size - size_used1)) + pointer_align));
      addr_pointed2 = *((void **)(((char*)s2->address + (s2->size - size_used2)) + pointer_align));
      if(is_heap_equality(equals, addr_pointed1, addr_pointed2) == 0){
        if((addr_pointed1 > std_heap) && (addr_pointed1 < (void *)((char *)std_heap + STD_HEAP_SIZE)) && (addr_pointed2 > std_heap) && (addr_pointed2 < (void *)((char *)std_heap + STD_HEAP_SIZE))){
          if(is_free_area(addr_pointed1, (xbt_mheap_t)heap1) == 0 || is_free_area(addr_pointed2, (xbt_mheap_t)heap2) == 0){
            return 1;
          }
        }else{
          return 1;
        } 
      }
    } 
    k++;
  }
 
  return 0;
}

int MC_compare_snapshots(void *s1, void *s2){

  return simcall_mc_compare_snapshots(s1, s2);

}
