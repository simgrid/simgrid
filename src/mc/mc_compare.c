/* Copyright (c) 2012-2013. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc_private.h"

#include "xbt/mmalloc.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_compare, mc,
                                "Logging specific to mc_compare");

typedef struct s_pointers_pair{
  void *p1;
  void *p2;
}s_pointers_pair_t, *pointers_pair_t;

__thread xbt_dynar_t compared_pointers;

/************************** Free functions ****************************/
/********************************************************************/

static void stack_region_free(stack_region_t s){
  if(s){
    xbt_free(s->process_name);
    xbt_free(s);
  }
}

static void stack_region_free_voidp(void *s){
  stack_region_free((stack_region_t) * (void **) s);
}

static void pointers_pair_free(pointers_pair_t p){
  xbt_free(p);
}

static void pointers_pair_free_voidp(void *p){
  pointers_pair_free((pointers_pair_t) * (void **)p);
}

/************************** Snapshot comparison *******************************/
/******************************************************************************/

static int already_compared_pointers(void *p1, void *p2){

  if(xbt_dynar_is_empty(compared_pointers))
    return -1;

  unsigned int cursor = 0;
  int start = 0;
  int end = xbt_dynar_length(compared_pointers) - 1;
  pointers_pair_t pair;

  while(start <= end){
    cursor = (start + end) / 2;
    pair = (pointers_pair_t)xbt_dynar_get_as(compared_pointers, cursor, pointers_pair_t);
    if(pair->p1 == p1){
      if(pair->p2 == p2)
        return 0;
      else if(pair->p2 < p2)
        start = cursor + 1;
      else
        end = cursor - 1;
    }else if(pair->p1 < p1){
      start = cursor + 1;
    }else{
      end = cursor - 1 ;
    }
  }

  return -1;

}

static void add_compared_pointers(void *p1, void *p2){

  pointers_pair_t new_pair = xbt_new0(s_pointers_pair_t, 1);
  new_pair->p1 = p1;
  new_pair->p2 = p2;
  
  if(xbt_dynar_is_empty(compared_pointers)){
    xbt_dynar_push(compared_pointers, &new_pair);
    return;
  }

  unsigned int cursor = 0;
  int start = 0;
  int end = xbt_dynar_length(compared_pointers) - 1;
  pointers_pair_t pair = NULL;

  while(start <= end){
    cursor = (start + end) / 2;
    pair = (pointers_pair_t)xbt_dynar_get_as(compared_pointers, cursor, pointers_pair_t);
    if(pair->p1 == p1){
      if(pair->p2 == p2){
        pointers_pair_free(new_pair);
        return;
      }else if(pair->p2 < p2)
        start = cursor + 1;
      else
        end = cursor - 1;
    }else if(pair->p1 < p1){
      start = cursor + 1;
    }else{
      end = cursor - 1 ;
    }
  }

  if(pair->p1 == p1){
    if(pair->p2 < p2)
      xbt_dynar_insert_at(compared_pointers, cursor + 1, &new_pair);
    else
      xbt_dynar_insert_at(compared_pointers, cursor, &new_pair); 
  }else{
    if(pair->p1 < p1)
      xbt_dynar_insert_at(compared_pointers, cursor + 1, &new_pair);
    else
      xbt_dynar_insert_at(compared_pointers, cursor, &new_pair);   
  }

}

static int compare_areas_with_type(void *area1, void *area2, xbt_dict_t types, xbt_dict_t other_types, char *type_id, int region_size, int region_type, void *start_data, int pointer_level){

  dw_type_t type = xbt_dict_get_or_null(types, type_id);;
  unsigned int cursor = 0;
  dw_type_t member, subtype, subsubtype;
  int elm_size, i, res, switch_types = 0;
  void *addr_pointed1, *addr_pointed2;

  switch(type->type){
  case e_dw_base_type:
  case e_dw_enumeration_type:
  case e_dw_union_type:
    return (memcmp(area1, area2, type->size) != 0);
    break;
  case e_dw_typedef:
  case e_dw_volatile_type:
    return compare_areas_with_type(area1, area2, types, other_types, type->dw_type_id, region_size, region_type, start_data, pointer_level);
    break;
  case e_dw_const_type: /* Const variable cannot be modified */
    return -1;
    break;
  case e_dw_array_type:
    subtype = xbt_dict_get_or_null(types, type->dw_type_id);
    switch(subtype->type){
    case e_dw_base_type:
    case e_dw_enumeration_type:
    case e_dw_pointer_type:
    case e_dw_structure_type:
    case e_dw_union_type:
      if(subtype->size == 0){ /*declaration of the type, need the complete description */
        subtype = xbt_dict_get_or_null(types, get_type_description(types, subtype->name));
        if(subtype == NULL){
          subtype = xbt_dict_get_or_null(other_types, get_type_description(other_types, subtype->name));
          switch_types = 1;
        }
      }
      elm_size = subtype->size;
      break;
    case e_dw_typedef:
    case e_dw_volatile_type:
      subsubtype = xbt_dict_get_or_null(types, subtype->dw_type_id);
      if(subsubtype->size == 0){ /*declaration of the type, need the complete description */
        subsubtype = xbt_dict_get_or_null(types, get_type_description(types, subsubtype->name));
        if(subsubtype == NULL){
          subsubtype = xbt_dict_get_or_null(other_types, get_type_description(other_types, subsubtype->name));
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
        res = compare_areas_with_type((char *)area1 + (i*elm_size), (char *)area2 + (i*elm_size), other_types, types, type->dw_type_id, region_size, region_type, start_data, pointer_level);
      else
        res = compare_areas_with_type((char *)area1 + (i*elm_size), (char *)area2 + (i*elm_size), types, other_types, type->dw_type_id, region_size, region_type, start_data, pointer_level);
      if(res == 1)
        return res;
    }
    break;
  case e_dw_pointer_type: 
    if(type->dw_type_id && ((dw_type_t)xbt_dict_get_or_null(types, type->dw_type_id))->type == e_dw_subroutine_type){
      addr_pointed1 = *((void **)(area1)); 
      addr_pointed2 = *((void **)(area2));
      return (addr_pointed1 != addr_pointed2);
    }else{
      addr_pointed1 = *((void **)(area1)); 
      addr_pointed2 = *((void **)(area2));
      
      if(addr_pointed1 == NULL && addr_pointed2 == NULL)
        return 0;
      if(already_compared_pointers(addr_pointed1, addr_pointed2) != -1)
        return 0;
      add_compared_pointers(addr_pointed1, addr_pointed2);

      pointer_level++;
      
      if(addr_pointed1 > std_heap && (char *)addr_pointed1 < (char*) std_heap + STD_HEAP_SIZE && addr_pointed2 > std_heap && (char *)addr_pointed2 < (char*) std_heap + STD_HEAP_SIZE){
        return compare_heap_area(addr_pointed1, addr_pointed2, NULL, types, other_types, type->dw_type_id, pointer_level); 
      }else if(addr_pointed1 > start_data && (char*)addr_pointed1 <= (char *)start_data + region_size && addr_pointed2 > start_data && (char*)addr_pointed2 <= (char *)start_data + region_size){
        if(type->dw_type_id == NULL)
          return  (addr_pointed1 != addr_pointed2);
        else
          return  compare_areas_with_type(addr_pointed1, addr_pointed2, types, other_types, type->dw_type_id, region_size, region_type, start_data, pointer_level); 
      }else{
        return (addr_pointed1 != addr_pointed2);
      }
    }
    break;
  case e_dw_structure_type:
    xbt_dynar_foreach(type->members, cursor, member){
      res = compare_areas_with_type((char *)area1 + member->offset, (char *)area2 + member->offset, types, other_types, member->dw_type_id, region_size, region_type, start_data, pointer_level);
      if(res == 1)
        return res;
    }
    break;
  case e_dw_subroutine_type:
    return -1;
    break;
  default:
    XBT_VERB("Unknown case : %d", type->type);
    break;
  }
  
  return 0;
}

static int compare_global_variables(int region_type, mc_mem_region_t r1, mc_mem_region_t r2){

  if(!compared_pointers){
    compared_pointers = xbt_dynar_new(sizeof(pointers_pair_t), pointers_pair_free_voidp);
    MC_ignore_global_variable("compared_pointers");
  }else{
    xbt_dynar_reset(compared_pointers);
  }

  xbt_dynar_t variables;
  xbt_dict_t types, other_types;
  int res;
  unsigned int cursor = 0;
  dw_variable_t current_var;
  size_t offset;
  void *start_data;

  if(region_type == 2){
    variables = mc_global_variables_binary;
    types = mc_variables_type_binary;
    other_types = mc_variables_type_libsimgrid;
    start_data = start_data_binary;
  }else{
    variables = mc_global_variables_libsimgrid;
    types = mc_variables_type_libsimgrid;
    other_types = mc_variables_type_binary;
    start_data = start_data_libsimgrid;
  }

  xbt_dynar_foreach(variables, cursor, current_var){

    if(region_type == 2)
      offset = (char *)current_var->address.address - (char *)start_data_binary;
    else
      offset = (char *)current_var->address.address - (char *)start_data_libsimgrid;

    res = compare_areas_with_type((char *)r1->data + offset, (char *)r2->data + offset, types, other_types, current_var->type_origin, r1->size, region_type, start_data, 0);
    if(res == 1){
      XBT_VERB("Global variable %s (%p - %p) is different between snapshots", current_var->name, (char *)r1->data + offset, (char *)r2->data + offset);
      xbt_dynar_free(&compared_pointers);
      compared_pointers = NULL;
      return 1;
    }

  }

  xbt_dynar_free(&compared_pointers);
  compared_pointers = NULL;

  return 0;

}

static int compare_local_variables(mc_snapshot_stack_t stack1, mc_snapshot_stack_t stack2, void *heap1, void *heap2){

  if(!compared_pointers){
    compared_pointers = xbt_dynar_new(sizeof(pointers_pair_t), pointers_pair_free_voidp);
    MC_ignore_global_variable("compared_pointers");
  }else{
    xbt_dynar_reset(compared_pointers);
  }

  if(xbt_dynar_length(stack1->local_variables) != xbt_dynar_length(stack2->local_variables)){
    XBT_VERB("Different number of local variables");
    xbt_dynar_free(&compared_pointers);
    compared_pointers = NULL;
    return 1;
  }else{
    unsigned int cursor = 0;
    local_variable_t current_var1, current_var2;
    int offset1, offset2, res;
    while(cursor < xbt_dynar_length(stack1->local_variables)){
      current_var1 = (local_variable_t)xbt_dynar_get_as(stack1->local_variables, cursor, local_variable_t);
      current_var2 = (local_variable_t)xbt_dynar_get_as(stack2->local_variables, cursor, local_variable_t);
      if(strcmp(current_var1->name, current_var2->name) != 0 || strcmp(current_var1->frame, current_var2->frame) != 0 || current_var1->ip != current_var2->ip){
        xbt_dynar_free(&compared_pointers);
        return 1;
      }
      offset1 = (char *)current_var1->address - (char *)std_heap;
      offset2 = (char *)current_var2->address - (char *)std_heap;
      if(current_var1->region == 1)
        res = compare_areas_with_type( (char *)heap1 + offset1, (char *)heap2 + offset2, mc_variables_type_libsimgrid, mc_variables_type_binary, current_var1->type, 0, 1, start_data_libsimgrid, 0);
      else
        res = compare_areas_with_type( (char *)heap1 + offset1, (char *)heap2 + offset2, mc_variables_type_binary, mc_variables_type_libsimgrid, current_var1->type, 0, 2, start_data_binary, 0);
      if(res == 1){
        XBT_VERB("Local variable %s (%p - %p) in frame %s  is different between snapshots", current_var1->name,(char *)heap1 + offset1, (char *)heap2 + offset2, current_var1->frame);
        xbt_dynar_free(&compared_pointers);
        compared_pointers = NULL;
        return res;
      }
      cursor++;
    }
    xbt_dynar_free(&compared_pointers);
    compared_pointers = NULL;
    return 0;
  }
}

int snapshot_compare(void *state1, void *state2){

  mc_snapshot_t s1, s2;
  int num1, num2;
  
  if(_sg_mc_property_file && _sg_mc_property_file[0] != '\0'){ /* Liveness MC */
    s1 = ((mc_pair_t)state1)->graph_state->system_state;
    s2 = ((mc_pair_t)state2)->graph_state->system_state;
    num1 = ((mc_pair_t)state1)->num;
    num2 =  ((mc_pair_t)state2)->num;
    /* Firstly compare automaton state */
    if(xbt_automaton_state_compare(((mc_pair_t)state1)->automaton_state, ((mc_pair_t)state2)->automaton_state) != 0)
      return 1;
    if(xbt_automaton_propositional_symbols_compare_value(((mc_pair_t)state1)->atomic_propositions, ((mc_pair_t)state2)->atomic_propositions) != 0)
      return 1;
  }else{ /* Safety MC */
    s1 = ((mc_visited_state_t)state1)->system_state;
    s2 = ((mc_visited_state_t)state2)->system_state;
    num1 = ((mc_visited_state_t)state1)->num;
    num2 = ((mc_visited_state_t)state2)->num;
  }

  int errors = 0;
  int res_init;

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
      XBT_DEBUG("(%d - %d) Different size used in stacks : %zu - %zu", num1, num2, size_used1, size_used2);
      errors++;
      is_diff = 1;
    #else
      #ifdef MC_VERBOSE
      XBT_VERB("(%d - %d) Different size used in stacks : %zu - %zu", num1, num2, size_used1, size_used2);
      #endif

      xbt_os_walltimer_stop(timer);
      xbt_os_timer_free(timer);
      xbt_os_walltimer_stop(global_timer);
      mc_snapshot_comparison_time = xbt_os_timer_elapsed(global_timer);
      xbt_os_timer_free(global_timer);

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

        xbt_os_walltimer_stop(timer);
        xbt_os_timer_free(timer);
        xbt_os_walltimer_stop(global_timer);
        mc_snapshot_comparison_time = xbt_os_timer_elapsed(global_timer);
        xbt_os_timer_free(global_timer);

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

        xbt_os_walltimer_stop(timer);
        xbt_os_timer_free(timer);
        xbt_os_walltimer_stop(global_timer);
        mc_snapshot_comparison_time = xbt_os_timer_elapsed(global_timer);
        xbt_os_timer_free(global_timer);

        return 1;
      #endif
    }
  }

  #ifdef MC_DEBUG
    xbt_os_walltimer_start(timer);
  #endif

  /* Init heap information used in heap comparison algorithm */
  res_init = init_heap_information((xbt_mheap_t)s1->regions[0]->data, (xbt_mheap_t)s2->regions[0]->data, s1->to_ignore, s2->to_ignore);
  if(res_init == -1){
     #ifdef MC_DEBUG
    XBT_DEBUG("(%d - %d) Different heap information", num1, num2); 
        errors++; 
      #else
        #ifdef MC_VERBOSE
        XBT_VERB("(%d - %d) Different heap information", num1, num2); 
        #endif

        xbt_os_walltimer_stop(global_timer);
        mc_snapshot_comparison_time = xbt_os_timer_elapsed(global_timer);
        xbt_os_timer_free(global_timer);

        return 1;
      #endif
  }

  #ifdef MC_DEBUG
    xbt_os_walltimer_start(timer);
  #endif

  /* Stacks comparison */
  unsigned int  cursor = 0;
  int diff_local = 0;
  is_diff = 0;
  mc_snapshot_stack_t stack1, stack2;
    
  while(cursor < xbt_dynar_length(s1->stacks)){
    stack1 = (mc_snapshot_stack_t)xbt_dynar_get_as(s1->stacks, cursor, mc_snapshot_stack_t);
    stack2 = (mc_snapshot_stack_t)xbt_dynar_get_as(s2->stacks, cursor, mc_snapshot_stack_t);
    diff_local = compare_local_variables(stack1, stack2, s1->regions[0]->data, s2->regions[0]->data);
    if(diff_local > 0){
      #ifdef MC_DEBUG
        if(is_diff == 0){
          xbt_os_walltimer_stop(timer);
          mc_comp_times->stacks_comparison_time = xbt_os_timer_elapsed(timer);
        }
        XBT_DEBUG("(%d - %d) Different local variables between stacks %d", num1, num2, cursor + 1);
        errors++;
        is_diff = 1;
      #else
        
        #ifdef MC_VERBOSE
        XBT_VERB("(%d - %d) Different local variables between stacks %d", num1, num2, cursor + 1);
        #endif
          
        reset_heap_information();
        xbt_os_walltimer_stop(timer);
        xbt_os_timer_free(timer);
        xbt_os_walltimer_stop(global_timer);
        mc_snapshot_comparison_time = xbt_os_timer_elapsed(global_timer);
        xbt_os_timer_free(global_timer);
 
        return 1;
      #endif
    }
    cursor++;
  }

  #ifdef MC_DEBUG
    if(is_diff == 0)
      xbt_os_walltimer_stop(timer);
    xbt_os_walltimer_start(timer);
  #endif

  /* Compare binary global variables */
  is_diff = compare_global_variables(2, s1->regions[2], s2->regions[2]);
  if(is_diff != 0){
    #ifdef MC_DEBUG
      xbt_os_walltimer_stop(timer);
      mc_comp_times->binary_global_variables_comparison_time = xbt_os_timer_elapsed(timer);
      XBT_DEBUG("(%d - %d) Different global variables in binary", num1, num2);
      errors++;
    #else
      #ifdef MC_VERBOSE
      XBT_VERB("(%d - %d) Different global variables in binary", num1, num2);
      #endif

      reset_heap_information();
      xbt_os_walltimer_stop(timer);
      xbt_os_timer_free(timer);
      xbt_os_walltimer_stop(global_timer);
      mc_snapshot_comparison_time = xbt_os_timer_elapsed(global_timer);
      xbt_os_timer_free(global_timer);

      return 1;
    #endif
  }

  #ifdef MC_DEBUG
    if(is_diff == 0)
      xbt_os_walltimer_stop(timer);
    xbt_os_walltimer_start(timer);
  #endif

  /* Compare libsimgrid global variables */
  is_diff = compare_global_variables(1, s1->regions[1], s2->regions[1]);
  if(is_diff != 0){
    #ifdef MC_DEBUG
      xbt_os_walltimer_stop(timer);
      mc_comp_times->libsimgrid_global_variables_comparison_time = xbt_os_timer_elapsed(timer);
      XBT_DEBUG("(%d - %d) Different global variables in libsimgrid", num1, num2);
      errors++;
    #else
      #ifdef MC_VERBOSE
      XBT_VERB("(%d - %d) Different global variables in libsimgrid", num1, num2);
      #endif
        
      reset_heap_information();
      xbt_os_walltimer_stop(timer);
      xbt_os_timer_free(timer);
      xbt_os_walltimer_stop(global_timer);
      mc_snapshot_comparison_time = xbt_os_timer_elapsed(global_timer);
      xbt_os_timer_free(global_timer);

      return 1;
    #endif
  }

  #ifdef MC_DEBUG
    xbt_os_walltimer_start(timer);
  #endif

  /* Compare heap */
  if(mmalloc_compare_heap((xbt_mheap_t)s1->regions[0]->data, (xbt_mheap_t)s2->regions[0]->data) > 0){

    #ifdef MC_DEBUG
      xbt_os_walltimer_stop(timer);
      mc_comp_times->heap_comparison_time = xbt_os_timer_elapsed(timer); 
      XBT_DEBUG("(%d - %d) Different heap (mmalloc_compare)", num1, num2);
      errors++;
    #else
 
      #ifdef MC_VERBOSE
      XBT_VERB("(%d - %d) Different heap (mmalloc_compare)", num1, num2);
      #endif
       
      reset_heap_information();
      xbt_os_walltimer_stop(timer);
      xbt_os_timer_free(timer);
      xbt_os_walltimer_stop(global_timer);
      mc_snapshot_comparison_time = xbt_os_timer_elapsed(global_timer);
      xbt_os_timer_free(global_timer);

      return 1;
    #endif
  }else{
    #ifdef MC_DEBUG
      xbt_os_walltimer_stop(timer);
    #endif
  }

  reset_heap_information();
  
  xbt_os_walltimer_stop(timer);
  xbt_os_timer_free(timer);

  #ifdef MC_VERBOSE
    xbt_os_walltimer_stop(global_timer);
    mc_snapshot_comparison_time = xbt_os_timer_elapsed(global_timer);
  #endif

  xbt_os_timer_free(global_timer);

  #ifdef MC_DEBUG
    print_comparison_times();
  #endif

  return errors > 0;
  
}

/***************************** Statistics *****************************/
/*******************************************************************/

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

/**************************** MC snapshot compare simcall **************************/
/***********************************************************************************/

int SIMIX_pre_mc_compare_snapshots(smx_simcall_t simcall,
                                   mc_snapshot_t s1, mc_snapshot_t s2){
  return snapshot_compare(s1, s2);
}

int MC_compare_snapshots(void *s1, void *s2){
  
  MC_ignore_local_variable("self", "simcall_BODY_mc_snapshot");
  return simcall_mc_compare_snapshots(s1, s2);

}
