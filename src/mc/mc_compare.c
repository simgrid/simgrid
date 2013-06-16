/* Copyright (c) 2012-2013 Da SimGrid Team. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc_private.h"

#include "xbt/mmalloc.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_compare, mc,
                                "Logging specific to mc_compare");

static int heap_region_compare(void *d1, void *d2, size_t size);

static int is_heap_equality(xbt_dynar_t equals, void *a1, void *a2);
static size_t heap_ignore_size(void *address);

static void stack_region_free(stack_region_t s);
static void heap_equality_free(heap_equality_t e);

static int compare_local_variables(char *s1, char *s2);
static int compare_global_variables(int region_type, void *d1, void *d2);

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

static int compare_global_variables(int region_type, void *d1, void *d2){ /* region_type = 1 -> libsimgrid, region_type = 2 -> binary */

  unsigned int cursor = 0;
  size_t offset; 
  int i = 0;
  global_variable_t current_var; 
  int pointer_align; 
  void *addr_pointed1 = NULL, *addr_pointed2 = NULL;
  int res_compare = 0;
  void *plt_start, *plt_end;

  if(region_type == 2){
    plt_start = start_plt_binary;
    plt_end = end_plt_binary;
  }else{
    plt_start = start_plt_libsimgrid;
    plt_end = end_plt_libsimgrid;
  }

  xbt_dynar_foreach(mc_global_variables, cursor, current_var){
    if(current_var->address < start_data_libsimgrid){ /* binary global variable */
      if(region_type == 1)
        continue;
      offset = (char *)current_var->address - (char *)start_data_binary;
    }else{ /* libsimgrid global variable */
      if(region_type == 2)
        break;
      offset = (char *)current_var->address - (char *)start_data_libsimgrid;
    }
    i = 0;
    while(i < current_var->size){
      if(memcmp((char *)d1 + offset + i, (char *)d2 + offset + i, 1) != 0){ 
        pointer_align = (i / sizeof(void*)) * sizeof(void*); 
        addr_pointed1 = *((void **)((char *)d1 + offset + pointer_align));
        addr_pointed2 = *((void **)((char *)d2 + offset + pointer_align));
        if((addr_pointed1 > plt_start && addr_pointed1 < plt_end) || (addr_pointed2 > plt_start && addr_pointed2 < plt_end)){
          i = pointer_align + sizeof(void*);
          continue;
        }else if((addr_pointed1 > std_heap) && ((char *)addr_pointed1 < (char *)std_heap + STD_HEAP_SIZE) 
                && (addr_pointed2 > std_heap) && ((char *)addr_pointed2 < (char *)std_heap + STD_HEAP_SIZE)){
          res_compare = compare_area(addr_pointed1, addr_pointed2, NULL);
          if(res_compare == 1){
            #ifdef MC_VERBOSE
              if(region_type == 2)
                XBT_VERB("Different global variable in binary : %s at addresses %p - %p (size = %zu)", current_var->name, (char *)d1+offset, (char *)d2+offset, current_var->size);
              else
                XBT_VERB("Different global variable in libsimgrid : %s at addresses %p - %p (size = %zu)", current_var->name, (char *)d1+offset, (char *)d2+offset, current_var->size);
            #endif
            #ifdef MC_DEBUG
              if(region_type == 2)
                XBT_DEBUG("Different global variable in binary : %s at addresses %p - %p (size = %zu)", current_var->name, (char *)d1+offset, (char *)d2+offset, current_var->size);
              else
                XBT_DEBUG("Different global variable in libsimgrid : %s at addresses %p - %p (size = %zu)", current_var->name, (char *)d1+offset, (char *)d2+offset, current_var->size);
            #endif
            return 1;
          }
          i = pointer_align + sizeof(void*);
          continue;
        }else{
          #ifdef MC_VERBOSE
            if(region_type == 2)
              XBT_VERB("Different global variable in binary : %s at addresses %p - %p (size = %zu)", current_var->name, (char *)d1+offset, (char *)d2+offset, current_var->size);
            else
              XBT_VERB("Different global variable in libsimgrid : %s at addresses %p - %p (size = %zu)", current_var->name, (char *)d1+offset, (char *)d2+offset, current_var->size);
          #endif
          #ifdef MC_DEBUG
            if(region_type == 2)
              XBT_DEBUG("Different global variable in binary : %s at addresses %p - %p (size = %zu)", current_var->name, (char *)d1+offset, (char *)d2+offset, current_var->size);
            else
              XBT_DEBUG("Different global variable in libsimgrid : %s at addresses %p - %p (size = %zu)", current_var->name, (char *)d1+offset, (char *)d2+offset, current_var->size);
          #endif
          return 1;
        }              
      }
      i++;
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
  xbt_free(e);
}

void heap_equality_free_voidp(void *e){
  heap_equality_free((heap_equality_t) * (void **) e);
}

int SIMIX_pre_mc_compare_snapshots(smx_simcall_t simcall,
                                   mc_snapshot_t s1, mc_snapshot_t s2){
  return snapshot_compare(s1, s2);
}

int snapshot_compare(mc_snapshot_t s1, mc_snapshot_t s2){

  int raw_mem = (mmalloc_get_current_heap() == raw_heap);
  
  MC_SET_RAW_MEM;
     
  int errors = 0;

  xbt_os_timer_t global_timer = xbt_os_timer_new();
  xbt_os_timer_t timer = xbt_os_timer_new();

  xbt_os_walltimer_start(global_timer);

  #ifdef MC_DEBUG
    xbt_os_walltimer_start(timer);
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
        xbt_os_walltimer_stop(timer);
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
      xbt_os_walltimer_stop(global_timer);
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
      xbt_os_walltimer_stop(timer);
    xbt_os_walltimer_start(timer);
  #endif

  /* Compare hash of global variables */
  if(s1->hash_global != NULL && s2->hash_global != NULL){
    if(strcmp(s1->hash_global, s2->hash_global) != 0){
      #ifdef MC_DEBUG
        xbt_os_walltimer_stop(timer);
        mc_comp_times->hash_global_variables_comparison_time = xbt_os_timer_elapsed(timer);
        XBT_DEBUG("Different hash of global variables : %s - %s", s1->hash_global, s2->hash_global); 
        errors++; 
      #else
        #ifdef MC_VERBOSE
          XBT_VERB("Different hash of global variables : %s - %s", s1->hash_global, s2->hash_global); 
        #endif

        xbt_os_timer_free(timer);
        xbt_os_walltimer_stop(global_timer);
        mc_snapshot_comparison_time = xbt_os_timer_elapsed(global_timer);
        xbt_os_timer_free(global_timer);
    
        if(!raw_mem)
          MC_UNSET_RAW_MEM;

        return 1;
      #endif
    }
  }

  #ifdef MC_DEBUG
    xbt_os_walltimer_start(timer);
  #endif

  /* Compare hash of local variables */
  if(s1->hash_local != NULL && s2->hash_local != NULL){
    if(strcmp(s1->hash_local, s2->hash_local) != 0){
      #ifdef MC_DEBUG
        xbt_os_walltimer_stop(timer);
        mc_comp_times->hash_local_variables_comparison_time = xbt_os_timer_elapsed(timer);
        XBT_DEBUG("Different hash of local variables : %s - %s", s1->hash_local, s2->hash_local); 
        errors++; 
      #else
        #ifdef MC_VERBOSE
          XBT_VERB("Different hash of local variables : %s - %s", s1->hash_local, s2->hash_local); 
        #endif

        xbt_os_timer_free(timer);
        xbt_os_walltimer_stop(global_timer);
        mc_snapshot_comparison_time = xbt_os_timer_elapsed(global_timer);
        xbt_os_timer_free(global_timer);
        
        if(!raw_mem)
          MC_UNSET_RAW_MEM;
        
        return 1;
      #endif
    }
  }

  #ifdef MC_DEBUG
    xbt_os_walltimer_start(timer);
  #endif

  /* Init heap information used in heap comparison algorithm */
  init_heap_information((xbt_mheap_t)s1->regions[0]->data, (xbt_mheap_t)s2->regions[0]->data, s1->to_ignore, s2->to_ignore);

  /* Compare binary global variables */
  is_diff = compare_global_variables(2, s1->regions[2]->data, s2->regions[2]->data);
  if(is_diff != 0){
    #ifdef MC_DEBUG
      xbt_os_walltimer_stop(timer);
      mc_comp_times->binary_global_variables_comparison_time = xbt_os_timer_elapsed(timer);
      XBT_DEBUG("Different global variables in binary"); 
      errors++; 
    #else
      #ifdef MC_VERBOSE
        XBT_VERB("Different global variables in binary"); 
      #endif

      reset_heap_information();
      xbt_os_timer_free(timer);
      xbt_os_walltimer_stop(global_timer);
      mc_snapshot_comparison_time = xbt_os_timer_elapsed(global_timer);
      xbt_os_timer_free(global_timer);
    
      if(!raw_mem)
        MC_UNSET_RAW_MEM;

      return 1;
    #endif
  }

  #ifdef MC_DEBUG
    if(is_diff == 0)
      xbt_os_walltimer_stop(timer);
    xbt_os_walltimer_start(timer);
  #endif

  /* Compare libsimgrid global variables */
  is_diff = compare_global_variables(1, s1->regions[1]->data, s2->regions[1]->data);
  if(is_diff != 0){
    #ifdef MC_DEBUG
      xbt_os_walltimer_stop(timer);
      mc_comp_times->libsimgrid_global_variables_comparison_time = xbt_os_timer_elapsed(timer); 
      XBT_DEBUG("Different global variables in libsimgrid"); 
      errors++; 
    #else
      #ifdef MC_VERBOSE
        XBT_VERB("Different global variables in libsimgrid"); 
      #endif
        
      reset_heap_information();
      xbt_os_timer_free(timer);
      xbt_os_walltimer_stop(global_timer);
      mc_snapshot_comparison_time = xbt_os_timer_elapsed(global_timer);
      xbt_os_timer_free(global_timer);
    
      if(!raw_mem)
        MC_UNSET_RAW_MEM;

      return 1;
    #endif
  }  

  #ifdef MC_DEBUG
    if(is_diff == 0)
      xbt_os_walltimer_stop(timer);
    xbt_os_walltimer_start(timer);
  #endif

  /* Stacks comparison */
  unsigned int  cursor = 0;
  int diff_local = 0;
  is_diff = 0;
    
  while(cursor < xbt_dynar_length(s1->stacks)){
    diff_local = compare_local_variables(((mc_snapshot_stack_t)xbt_dynar_get_as(s1->stacks, cursor, mc_snapshot_stack_t))->local_variables->data, ((mc_snapshot_stack_t)xbt_dynar_get_as(s2->stacks, cursor, mc_snapshot_stack_t))->local_variables->data);
    if(diff_local > 0){
      #ifdef MC_DEBUG
        if(is_diff == 0){
          xbt_os_walltimer_stop(timer);
          mc_comp_times->stacks_comparison_time = xbt_os_timer_elapsed(timer); 
        }
        XBT_DEBUG("Different local variables between stacks %d", cursor + 1);
        errors++;
        is_diff = 1;
      #else
        
        #ifdef MC_VERBOSE
          XBT_VERB("Different local variables between stacks %d", cursor + 1);
        #endif
          
        reset_heap_information();
        xbt_os_timer_free(timer);
        xbt_os_walltimer_stop(global_timer);
        mc_snapshot_comparison_time = xbt_os_timer_elapsed(global_timer);
        xbt_os_timer_free(global_timer);
        
        if(!raw_mem)
          MC_UNSET_RAW_MEM;
        
        return 1;
      #endif
    }
    cursor++;
  }
    
  #ifdef MC_DEBUG
    xbt_os_walltimer_start(timer);
  #endif

  /* Compare heap */
  if(mmalloc_compare_heap((xbt_mheap_t)s1->regions[0]->data, (xbt_mheap_t)s2->regions[0]->data)){

    #ifdef MC_DEBUG
      xbt_os_walltimer_stop(timer);
      mc_comp_times->heap_comparison_time = xbt_os_timer_elapsed(timer); 
      XBT_DEBUG("Different heap (mmalloc_compare)");
      errors++;
    #else

      xbt_os_timer_free(timer);
 
      #ifdef MC_VERBOSE
        XBT_VERB("Different heap (mmalloc_compare)");
      #endif
       
      reset_heap_information();
      xbt_os_walltimer_stop(global_timer);
      mc_snapshot_comparison_time = xbt_os_timer_elapsed(global_timer);
      xbt_os_timer_free(global_timer);

      if(!raw_mem)
        MC_UNSET_RAW_MEM;

      return 1;
    #endif
  }else{
    #ifdef MC_DEBUG
      xbt_os_walltimer_stop(timer);
    #endif
  }

  reset_heap_information();
   
  xbt_os_timer_free(timer);

  #ifdef MC_VERBOSE
    xbt_os_walltimer_stop(global_timer);
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

int is_stack_ignore_variable(char *frame, char *var_name){

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

static int compare_local_variables(char *s1, char *s2){
  
  xbt_dynar_t tokens1 = xbt_str_split(s1, NULL);
  xbt_dynar_t tokens2 = xbt_str_split(s2, NULL);

  xbt_dynar_t s_tokens1, s_tokens2;
  unsigned int cursor = 0;
  void *addr1, *addr2;
  char *frame_name1 = NULL, *frame_name2 = NULL;
  int res_compare = 0;

  #if defined MC_VERBOSE || defined MC_DEBUG
    char *var_name;
  #endif

  while(cursor < xbt_dynar_length(tokens1)){
    s_tokens1 = xbt_str_split(xbt_dynar_get_as(tokens1, cursor, char *), "=");
    s_tokens2 = xbt_str_split(xbt_dynar_get_as(tokens2, cursor, char *), "=");
    if(xbt_dynar_length(s_tokens1) > 1 && xbt_dynar_length(s_tokens2) > 1){
      #if defined MC_VERBOSE || defined MC_DEBUG
        var_name = xbt_dynar_get_as(s_tokens1, 0, char *);
      #endif
      if((strcmp(xbt_dynar_get_as(s_tokens1, 0, char *), "frame_name") == 0) && (strcmp(xbt_dynar_get_as(s_tokens2, 0, char *), "frame_name") == 0)){
        xbt_free(frame_name1);
        xbt_free(frame_name2);
        frame_name1 = strdup(xbt_dynar_get_as(s_tokens1, 1, char *));
        frame_name2 = strdup(xbt_dynar_get_as(s_tokens2, 1, char *));
      }
      addr1 = (void *) strtoul(xbt_dynar_get_as(s_tokens1, 1, char *), NULL, 16);
      addr2 = (void *) strtoul(xbt_dynar_get_as(s_tokens2, 1, char *), NULL, 16);
      if(addr1 > std_heap && (char *)addr1 <= (char *)std_heap + STD_HEAP_SIZE && addr2 > std_heap && (char *)addr2 <= (char *)std_heap + STD_HEAP_SIZE){
        res_compare = compare_area(addr1, addr2, NULL);
        if(res_compare == 1){
          if(is_stack_ignore_variable(frame_name1, xbt_dynar_get_as(s_tokens1, 0, char *)) && is_stack_ignore_variable(frame_name2, xbt_dynar_get_as(s_tokens2, 0, char *))){
            xbt_dynar_free(&s_tokens1);
            xbt_dynar_free(&s_tokens2);
            cursor++;
            continue;
          }else {
            #ifdef MC_VERBOSE
              XBT_VERB("Different local variable : %s at addresses %p - %p in frame %s", var_name, addr1, addr2, frame_name1);
            #endif
            #ifdef MC_DEBUG
              XBT_DEBUG("Different local variable : %s at addresses %p - %p", var_name, addr1, addr2);
            #endif
            xbt_dynar_free(&s_tokens1);
            xbt_dynar_free(&s_tokens2);
            xbt_dynar_free(&tokens1);
            xbt_dynar_free(&tokens2);
            xbt_free(frame_name1);
            xbt_free(frame_name2);
            return 1;
          }
        }
      }else{
        if(strcmp(xbt_dynar_get_as(s_tokens1, 1, char *), xbt_dynar_get_as(s_tokens2, 1, char *)) != 0){
          if(is_stack_ignore_variable(frame_name1, xbt_dynar_get_as(s_tokens1, 0, char *)) && is_stack_ignore_variable(frame_name2, xbt_dynar_get_as(s_tokens2, 0, char *))){
            xbt_dynar_free(&s_tokens1);
            xbt_dynar_free(&s_tokens2);
            cursor++;
            continue;
          }else {
            #ifdef MC_VERBOSE
              XBT_VERB("Different local variable : %s (%s - %s) in frame %s", var_name, xbt_dynar_get_as(s_tokens1, 1, char *), xbt_dynar_get_as(s_tokens2, 1, char *), frame_name1);
            #endif
            #ifdef MC_DEBUG
              XBT_DEBUG("Different local variable : %s (%s - %s)", var_name, xbt_dynar_get_as(s_tokens1, 1, char *), xbt_dynar_get_as(s_tokens2, 1, char *));
            #endif

            xbt_dynar_free(&s_tokens1);
            xbt_dynar_free(&s_tokens2);
            xbt_dynar_free(&tokens1);
            xbt_dynar_free(&tokens2);
            xbt_free(frame_name1);
            xbt_free(frame_name2);
            return 1;
          }
        }
      }
    }
    xbt_dynar_free(&s_tokens1);
    xbt_dynar_free(&s_tokens2);
         
    cursor++;
  }

  xbt_free(frame_name1);
  xbt_free(frame_name2);
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
