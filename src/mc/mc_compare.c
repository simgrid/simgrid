/* Copyright (c) 2008-2012 Da SimGrid Team. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc_private.h"

#include "xbt/mmalloc.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_compare, mc,
                                "Logging specific to mc_compare");

static int heap_region_compare(void *d1, void *d2, size_t size);

static int compare_stack(stack_region_t s1, stack_region_t s2, void *sp1, void *sp2, void *heap1, void *heap2, xbt_dynar_t equals);
static int is_heap_equality(xbt_dynar_t equals, void *a1, void *a2);
static size_t heap_ignore_size(void *address);

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

static int compare_global_variables(int region_type, void *d1, void *d2, xbt_dynar_t equals){

  unsigned int cursor = 0;
  size_t offset; 
  int i = 0;
  global_variable_t current_var; 
  int pointer_align; 
  void *addr_pointed1 = NULL, *addr_pointed2 = NULL;
  int res_compare = 0;

  if(region_type == 1){ /* libsimgrid */
    xbt_dynar_foreach(mc_global_variables, cursor, current_var){
      if(current_var->address < start_data_libsimgrid)
        continue;
      offset = (char *)current_var->address - (char *)start_data_libsimgrid;
      i = 0;
      while(i < current_var->size){
        if(memcmp((char*)d1 + offset + i, (char*)d2 + offset + i, 1) != 0){
          pointer_align = (i / sizeof(void*)) * sizeof(void*); 
          addr_pointed1 = *((void **)((char *)d1 + offset + pointer_align));
          addr_pointed2 = *((void **)((char *)d2 + offset + pointer_align));
          if((addr_pointed1 > start_plt_libsimgrid && addr_pointed1 < end_plt_libsimgrid) || (addr_pointed2 > start_plt_libsimgrid && addr_pointed2 < end_plt_libsimgrid)){
            i = current_var->size;
            continue;
          }else{
            if((addr_pointed1 > std_heap) && ((char *)addr_pointed1 < (char *)std_heap + STD_HEAP_SIZE) && (addr_pointed2 > std_heap) && ((char *)addr_pointed2 < (char *)std_heap + STD_HEAP_SIZE)){
              res_compare = compare_area(addr_pointed1, addr_pointed2, NULL, equals);
              if(res_compare == 1){
                #ifdef MC_VERBOSE
                  XBT_VERB("Different global variable in libsimgrid : %s at addresses %p - %p (size = %zu)", current_var->name, (char *)d1+offset, (char *)d2+offset, current_var->size);
                #endif
                return 1;
              }
            }else{
              #ifdef MC_VERBOSE
                XBT_VERB("Different global variable in libsimgrid : %s at addresses %p - %p (size = %zu)", current_var->name, (char *)d1+offset, (char *)d2+offset, current_var->size);
              #endif
              return 1;
            }
           
          }
        } 
        i++;
      }
    }
  }else{ /* binary */
    xbt_dynar_foreach(mc_global_variables, cursor, current_var){
      if(current_var->address > start_data_libsimgrid)
        break;
      offset = (char *)current_var->address - (char *)start_data_binary;
      i = 0;
      while(i < current_var->size){
        if(memcmp((char*)d1 + offset + i, (char*)d2 + offset + i, 1) != 0){
          pointer_align = (i / sizeof(void*)) * sizeof(void*); 
          addr_pointed1 = *((void **)((char *)d1 + offset + pointer_align));
          addr_pointed2 = *((void **)((char *)d2 + offset + pointer_align));
          if((addr_pointed1 > start_plt_binary && addr_pointed1 < end_plt_binary) || (addr_pointed2 > start_plt_binary && addr_pointed2 < end_plt_binary)){
            i = current_var->size;
            continue;
          }else{
            if((addr_pointed1 > std_heap) && ((char *)addr_pointed1 < (char *)std_heap + STD_HEAP_SIZE) && (addr_pointed2 > std_heap) && ((char *)addr_pointed2 < (char *)std_heap + STD_HEAP_SIZE)){
              res_compare = compare_area(addr_pointed1, addr_pointed2, NULL, equals);
              if(res_compare == 1){
                #ifdef MC_VERBOSE
                  XBT_VERB("Different global variable in binary : %s at addresses %p - %p (size = %zu)", current_var->name, (char *)d1+offset, (char *)d2+offset, current_var->size);
                #endif
                return 1;
              }
            }else{
              #ifdef MC_VERBOSE
                XBT_VERB("Different global variable in binary : %s at addresses %p - %p (size = %zu)", current_var->name, (char *)d1+offset, (char *)d2+offset, current_var->size);
              #endif
              return 1;
            }
          }
        } 
        i++;
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

int SIMIX_pre_mc_compare_snapshots(smx_simcall_t simcall,
                                   mc_snapshot_t s1, mc_snapshot_t s2){
  return snapshot_compare(s1, s2);
}

int get_heap_region_index(mc_snapshot_t s){
  int i = 0;
  while(i < s->num_reg){
    switch(s->regions[i]->type){
    case 0:
      return i;
      break;
    default:
      i++;
      break;
    }
  }
  return -1;
}

int snapshot_compare(mc_snapshot_t s1, mc_snapshot_t s2){

  int raw_mem = (mmalloc_get_current_heap() == raw_heap);
  
  MC_SET_RAW_MEM;
     
  int errors = 0;

  xbt_os_timer_t global_timer = xbt_os_timer_new();
  xbt_os_timer_t timer = xbt_os_timer_new();

  xbt_os_timer_start(global_timer);

  #ifdef MC_DEBUG
    xbt_os_timer_start(timer);
  #endif

  /* Compare number of processes */
  if(s1->nb_processes != s2->nb_processes){
    #ifdef MC_DEBUG
      xbt_os_timer_stop(timer);
      mc_comp_times->nb_processes_comparison_time =  xbt_os_timer_elapsed(timer);
      XBT_DEBUG("Different number of processes : %d - %d", s1->nb_processes, s2->nb_processes);
      errors++;
    #else
      #ifdef MC_VERBOSE
        XBT_VERB("Different number of processes : %d - %d", s1->nb_processes, s2->nb_processes);
      #endif
     
      xbt_os_timer_free(timer);
      xbt_os_timer_stop(global_timer);
      mc_snapshot_comparison_time = xbt_os_timer_elapsed(global_timer);
      xbt_os_timer_free(global_timer);

      if(!raw_mem)
        MC_UNSET_RAW_MEM;

      return 1;
    #endif
  }

  #ifdef MC_DEBUG
    xbt_os_timer_start(timer);
  #endif

  /* Compare number of bytes used in each heap */
  if(s1->heap_bytes_used != s2->heap_bytes_used){
    #ifdef MC_DEBUG
      xbt_os_timer_stop(timer);
      mc_comp_times->bytes_used_comparison_time = xbt_os_timer_elapsed(timer);
      XBT_DEBUG("Different number of bytes used in each heap : %zu - %zu", s1->heap_bytes_used, s2->heap_bytes_used);
      errors++;
    #else
      #ifdef MC_VERBOSE
        XBT_VERB("Different number of bytes used in each heap : %zu - %zu", s1->heap_bytes_used, s2->heap_bytes_used);
      #endif

      xbt_os_timer_free(timer);
      xbt_os_timer_stop(global_timer);
      mc_snapshot_comparison_time = xbt_os_timer_elapsed(global_timer);
      xbt_os_timer_free(global_timer);

      if(!raw_mem)
        MC_UNSET_RAW_MEM;

      return 1;
    #endif
  }else{
    #ifdef MC_DEBUG
      xbt_os_timer_stop(timer);
    #endif
  }
  
  #ifdef MC_DEBUG
    xbt_os_timer_start(timer);
  #endif
  
  /* Compare size of stacks */
  int i = 0;
  size_t size_used1, size_used2;
  int is_diff = 0;
  while(i < xbt_dynar_length(s1->stacks)){
    size_used1 = s1->stack_sizes[i];
    size_used2 = s2->stack_sizes[i];
    if(size_used1 != size_used2){
    #ifdef MC_DEBUG
      if(is_diff == 0){
        xbt_os_timer_stop(timer);
        mc_comp_times->stacks_sizes_comparison_time = xbt_os_timer_elapsed(timer);
      }
      XBT_DEBUG("Different size used in stacks : %zu - %zu", size_used1, size_used2);
      errors++;
      is_diff = 1;
    #else
      #ifdef MC_VERBOSE
        XBT_VERB("Different size used in stacks : %zu - %zu", size_used1, size_used2);
      #endif
 
      xbt_os_timer_free(timer);
      xbt_os_timer_stop(global_timer);
      mc_snapshot_comparison_time = xbt_os_timer_elapsed(global_timer);
      xbt_os_timer_free(global_timer);

      if(!raw_mem)
        MC_UNSET_RAW_MEM;

      return 1;
    #endif  
    }
    i++;
  }

  #ifdef MC_DEBUG
    if(is_diff == 0)
      xbt_os_timer_stop(timer);
    xbt_os_timer_start(timer);
  #endif

  int heap_index = 0, data_libsimgrid_index = 0, data_program_index = 0;
  i = 0;
  
  /* Get index of regions */
  while(i < s1->num_reg){
    switch(s1->region_type[i]){
    case 0:
      heap_index = i;
      i++;
      break;
    case 1:
      data_libsimgrid_index = i;
      i++;
      while( i < s1->num_reg && s1->region_type[i] == 1)
        i++;
      break;
    case 2:
      data_program_index = i;
      i++;
      while( i < s1->num_reg && s1->region_type[i] == 2)
        i++;
      break;
    }
  }

  /* Init heap information used in heap comparison algorithm */
  init_heap_information((xbt_mheap_t)s1->regions[heap_index]->data, (xbt_mheap_t)s2->regions[heap_index]->data);

  xbt_dynar_t equals = xbt_dynar_new(sizeof(heap_equality_t), heap_equality_free_voidp);

  /* Compare binary global variables */
  is_diff = compare_global_variables(s1->region_type[data_program_index], s1->regions[data_program_index]->data, s2->regions[data_program_index]->data, equals);
  if(is_diff != 0){
    #ifdef MC_DEBUG
      xbt_os_timer_stop(timer);
      mc_comp_times->binary_global_variables_comparison_time = xbt_os_timer_elapsed(timer);
      XBT_DEBUG("Different global variables in binary"); 
      errors++; 
    #else
      #ifdef MC_VERBOSE
        XBT_VERB("Different global variables in binary"); 
      #endif
    
      xbt_os_timer_free(timer);
      xbt_os_timer_stop(global_timer);
      mc_snapshot_comparison_time = xbt_os_timer_elapsed(global_timer);
      xbt_os_timer_free(global_timer);
    
      if(!raw_mem)
        MC_UNSET_RAW_MEM;

      return 1;
    #endif
  }

  #ifdef MC_VERBOSE
    if(is_diff == 0)
      xbt_os_timer_stop(timer);
    xbt_os_timer_start(timer);
  #endif

  /* Compare libsimgrid global variables */
    is_diff = compare_global_variables(s1->region_type[data_libsimgrid_index], s1->regions[data_libsimgrid_index]->data, s2->regions[data_libsimgrid_index]->data, equals);
  if(is_diff != 0){
    #ifdef MC_DEBUG
      xbt_os_timer_stop(timer);
      mc_comp_times->libsimgrid_global_variables_comparison_time = xbt_os_timer_elapsed(timer); 
      XBT_DEBUG("Different global variables in libsimgrid"); 
      errors++; 
    #else
      #ifdef MC_VERBOSE
        XBT_VERB("Different global variables in libsimgrid"); 
      #endif
    
      xbt_os_timer_free(timer);
      xbt_os_timer_stop(global_timer);
      mc_snapshot_comparison_time = xbt_os_timer_elapsed(global_timer);
      xbt_os_timer_free(global_timer);
    
      if(!raw_mem)
        MC_UNSET_RAW_MEM;

      return 1;
    #endif
  }  

  #ifdef MC_DEBUG
    if(is_diff == 0)
      xbt_os_timer_stop(timer);
    xbt_os_timer_start(timer);
  #endif

  /* Compare heap */
  xbt_dynar_t stacks1 = xbt_dynar_new(sizeof(stack_region_t), stack_region_free_voidp);
  xbt_dynar_t stacks2 = xbt_dynar_new(sizeof(stack_region_t), stack_region_free_voidp);
 
  if(mmalloc_compare_heap((xbt_mheap_t)s1->regions[heap_index]->data, (xbt_mheap_t)s2->regions[heap_index]->data, &stacks1, &stacks2, equals)){

    #ifdef MC_DEBUG
      xbt_os_timer_stop(timer);
      mc_comp_times->heap_comparison_time = xbt_os_timer_elapsed(timer); 
      XBT_DEBUG("Different heap (mmalloc_compare)");
      errors++;
    #else

      xbt_os_timer_free(timer);
      xbt_dynar_free(&stacks1);
      xbt_dynar_free(&stacks2);
      xbt_dynar_free(&equals);
 
      #ifdef MC_VERBOSE
        XBT_VERB("Different heap (mmalloc_compare)");
      #endif
       
      xbt_os_timer_stop(global_timer);
      mc_snapshot_comparison_time = xbt_os_timer_elapsed(global_timer);
      xbt_os_timer_free(global_timer);

      if(!raw_mem)
        MC_UNSET_RAW_MEM;

      return 1;
    #endif
  }else{
    #ifdef MC_DEBUG
      xbt_os_timer_stop(timer);
    #endif
  }

  #ifdef MC_DEBUG
    xbt_os_timer_start(timer);
  #endif

  /* Stacks comparison */
  unsigned int  cursor = 0;
  int diff_local = 0;
  is_diff = 0;

  while(cursor < xbt_dynar_length(stacks1)){
    diff_local = compare_local_variables(((mc_snapshot_stack_t)xbt_dynar_get_as(s1->stacks, cursor, mc_snapshot_stack_t))->local_variables->data, ((mc_snapshot_stack_t)xbt_dynar_get_as(s2->stacks, cursor, mc_snapshot_stack_t))->local_variables->data, equals);
    if(diff_local > 0){
      #ifdef MC_DEBUG
        if(is_diff == 0){
          xbt_os_timer_stop(timer);
          mc_comp_times->stacks_comparison_time = xbt_os_timer_elapsed(timer); 
        }
        XBT_DEBUG("Different local variables between stacks %d", cursor + 1);
        errors++;
        is_diff = 1;
      #else
        xbt_dynar_free(&stacks1);
        xbt_dynar_free(&stacks2);
        xbt_dynar_free(&equals);
        
      #ifdef MC_VERBOSE
        XBT_VERB("Different local variables between stacks %d", cursor + 1);
      #endif
          
        xbt_os_timer_free(timer);
        xbt_os_timer_stop(global_timer);
        mc_snapshot_comparison_time = xbt_os_timer_elapsed(global_timer);
        xbt_os_timer_free(global_timer);
        
        if(!raw_mem)
          MC_UNSET_RAW_MEM;
        
        return 1;
      #endif
    }
    cursor++;
  }

  xbt_dynar_free(&stacks1);
  xbt_dynar_free(&stacks2);
  xbt_dynar_free(&equals);
   
  xbt_os_timer_free(timer);

  #ifdef MC_VERBOSE
    xbt_os_timer_stop(global_timer);
    mc_snapshot_comparison_time = xbt_os_timer_elapsed(global_timer);
  #endif

  xbt_os_timer_free(global_timer);

  #ifdef MC_DEBUG
    print_comparison_times();
  #endif

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
        xbt_free(ip1);
        xbt_free(ip2);
        ip1 = strdup(xbt_dynar_get_as(s_tokens1, 1, char *));
        ip2 = strdup(xbt_dynar_get_as(s_tokens2, 1, char *));
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
            XBT_VERB("Variable %s is different between stacks in %s : %s - %s", xbt_dynar_get_as(s_tokens1, 0, char *), ip1, xbt_dynar_get_as(s_tokens1, 1, char *), xbt_dynar_get_as(s_tokens2, 1, char *));
          xbt_dynar_free(&s_tokens1);
          xbt_dynar_free(&s_tokens2);
          xbt_dynar_free(&tokens1);
          xbt_dynar_free(&tokens2);
          xbt_free(ip1);
          xbt_free(ip2);
          return 1;
        }
      }
    }
    xbt_dynar_free(&s_tokens1);
    xbt_dynar_free(&s_tokens2);
         
    cursor++;
  }

  xbt_free(ip1);
  xbt_free(ip2);
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
  
  MC_ignore_stack("self", "simcall_BODY_mc_snapshot");

  return simcall_mc_compare_snapshots(s1, s2);

}

void print_comparison_times(){
  XBT_DEBUG("*** Comparison times ***");
  XBT_DEBUG("- Nb processes : %f", mc_comp_times->nb_processes_comparison_time);
  XBT_DEBUG("- Nb bytes used : %f", mc_comp_times->bytes_used_comparison_time);
  XBT_DEBUG("- Stacks sizes : %f", mc_comp_times->stacks_sizes_comparison_time);
  XBT_DEBUG("- Binary global variables : %f", mc_comp_times->binary_global_variables_comparison_time);
  XBT_DEBUG("- Libsimgrid global variables : %f", mc_comp_times->libsimgrid_global_variables_comparison_time);
  XBT_DEBUG("- Heap : %f", mc_comp_times->heap_comparison_time);
  XBT_DEBUG("- Stacks : %f", mc_comp_times->stacks_comparison_time);
}
