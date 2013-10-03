/* Copyright (c) 2008-2013. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>

#include "simgrid/sg_config.h"
#include "../surf/surf_private.h"
#include "../simix/smx_private.h"
#include "../xbt/mmalloc/mmprivate.h"
#include "xbt/fifo.h"
#include "mc_private.h"
#include "xbt/automaton.h"
#include "xbt/dict.h"

XBT_LOG_NEW_CATEGORY(mc, "All MC categories");
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_global, mc,
                                "Logging specific to MC (global)");

/* Configuration support */
e_mc_reduce_t mc_reduce_kind=e_mc_reduce_unset;

int _sg_do_model_check = 0;
int _sg_mc_checkpoint=0;
char* _sg_mc_property_file=NULL;
int _sg_mc_timeout=0;
int _sg_mc_max_depth=1000;
int _sg_mc_visited=0;
char *_sg_mc_dot_output_file = NULL;

int user_max_depth_reached = 0;

void _mc_cfg_cb_reduce(const char *name, int pos) {
  if (_sg_cfg_init_status && !_sg_do_model_check) {
    xbt_die("You are specifying a reduction strategy after the initialization (through MSG_config?), but model-checking was not activated at config time (through --cfg=model-check:1). This won't work, sorry.");
  }
  char *val= xbt_cfg_get_string(_sg_cfg_set, name);
  if (!strcasecmp(val,"none")) {
    mc_reduce_kind = e_mc_reduce_none;
  } else if (!strcasecmp(val,"dpor")) {
    mc_reduce_kind = e_mc_reduce_dpor;
  } else {
    xbt_die("configuration option %s can only take 'none' or 'dpor' as a value",name);
  }
}

void _mc_cfg_cb_checkpoint(const char *name, int pos) {
  if (_sg_cfg_init_status && !_sg_do_model_check) {
    xbt_die("You are specifying a checkpointing value after the initialization (through MSG_config?), but model-checking was not activated at config time (through --cfg=model-check:1). This won't work, sorry.");
  }
  _sg_mc_checkpoint = xbt_cfg_get_int(_sg_cfg_set, name);
}
void _mc_cfg_cb_property(const char *name, int pos) {
  if (_sg_cfg_init_status && !_sg_do_model_check) {
    xbt_die("You are specifying a property after the initialization (through MSG_config?), but model-checking was not activated at config time (through --cfg=model-check:1). This won't work, sorry.");
  }
  _sg_mc_property_file= xbt_cfg_get_string(_sg_cfg_set, name);
}

void _mc_cfg_cb_timeout(const char *name, int pos) {
  if (_sg_cfg_init_status && !_sg_do_model_check) {
    xbt_die("You are specifying a value to enable/disable timeout for wait requests after the initialization (through MSG_config?), but model-checking was not activated at config time (through --cfg=model-check:1). This won't work, sorry.");
  }
  _sg_mc_timeout= xbt_cfg_get_boolean(_sg_cfg_set, name);
}

void _mc_cfg_cb_max_depth(const char *name, int pos) {
  if (_sg_cfg_init_status && !_sg_do_model_check) {
    xbt_die("You are specifying a max depth value after the initialization (through MSG_config?), but model-checking was not activated at config time (through --cfg=model-check:1). This won't work, sorry.");
  }
  _sg_mc_max_depth= xbt_cfg_get_int(_sg_cfg_set, name);
}

void _mc_cfg_cb_visited(const char *name, int pos) {
  if (_sg_cfg_init_status && !_sg_do_model_check) {
    xbt_die("You are specifying a number of stored visited states after the initialization (through MSG_config?), but model-checking was not activated at config time (through --cfg=model-check:1). This won't work, sorry.");
  }
  _sg_mc_visited= xbt_cfg_get_int(_sg_cfg_set, name);
}

void _mc_cfg_cb_dot_output(const char *name, int pos) {
  if (_sg_cfg_init_status && !_sg_do_model_check) {
    xbt_die("You are specifying a file name for a dot output of graph state after the initialization (through MSG_config?), but model-checking was not activated at config time (through --cfg=model-check:1). This won't work, sorry.");
  }
  _sg_mc_dot_output_file= xbt_cfg_get_string(_sg_cfg_set, name);
}

/* MC global data structures */
mc_state_t mc_current_state = NULL;
char mc_replay_mode = FALSE;
double *mc_time = NULL;
__thread mc_comparison_times_t mc_comp_times = NULL;
__thread double mc_snapshot_comparison_time;
mc_stats_t mc_stats = NULL;

/* Safety */
xbt_fifo_t mc_stack_safety = NULL;
mc_global_t initial_state_safety = NULL;

/* Liveness */
xbt_fifo_t mc_stack_liveness = NULL;
mc_global_t initial_state_liveness = NULL;
int compare;

xbt_automaton_t _mc_property_automaton = NULL;

/* Variables */
xbt_dict_t mc_local_variables_libsimgrid = NULL;
xbt_dict_t mc_local_variables_binary = NULL;
xbt_dynar_t mc_global_variables_libsimgrid = NULL;
xbt_dynar_t mc_global_variables_binary = NULL;
xbt_dict_t mc_variables_type_libsimgrid = NULL;
xbt_dict_t mc_variables_type_binary = NULL;

/* Ignore mechanism */
xbt_dynar_t mc_stack_comparison_ignore;
xbt_dynar_t mc_data_bss_comparison_ignore;
extern xbt_dynar_t mc_heap_comparison_ignore;
extern xbt_dynar_t stacks_areas;

/* Dot output */
FILE *dot_output = NULL;
const char* colors[13];


/*******************************  DWARF Information *******************************/
/**********************************************************************************/

/************************** Free functions *************************/

static void dw_location_free(dw_location_t l){
  if(l){
    if(l->type == e_dw_loclist)
      xbt_dynar_free(&(l->location.loclist));
    else if(l->type == e_dw_compose)
      xbt_dynar_free(&(l->location.compose));
    else if(l->type == e_dw_arithmetic)
      xbt_free(l->location.arithmetic);
  
    xbt_free(l);
  }
}

static void dw_location_entry_free(dw_location_entry_t e){
  dw_location_free(e->location);
  xbt_free(e);
}

static void dw_type_free(dw_type_t t){
  xbt_free(t->name);
  xbt_free(t->dw_type_id);
  xbt_dynar_free(&(t->members));
  xbt_free(t);
}

static void dw_type_free_voidp(void *t){
  dw_type_free((dw_type_t) * (void **) t);
}

static void dw_variable_free(dw_variable_t v){
  if(v){
    xbt_free(v->name);
    xbt_free(v->type_origin);
    if(!v->global)
      dw_location_free(v->address.location);
    xbt_free(v);
  }
}

static void dw_variable_free_voidp(void *t){
  dw_variable_free((dw_variable_t) * (void **) t);
}

/*************************************************************************/

static dw_location_t MC_dwarf_get_location(xbt_dict_t location_list, char *expr){

  dw_location_t loc = xbt_new0(s_dw_location_t, 1);

  if(location_list != NULL){
    
    char *key = bprintf("%d", (int)strtoul(expr, NULL, 16));
    loc->type = e_dw_loclist;
    loc->location.loclist =  (xbt_dynar_t)xbt_dict_get_or_null(location_list, key);
    if(loc->location.loclist == NULL)
      XBT_INFO("Key not found in loclist");
    xbt_free(key);
    return loc;

  }else{

    int cursor = 0;
    char *tok = NULL, *tok2 = NULL; 
    
    xbt_dynar_t tokens1 = xbt_str_split(expr, ";");
    xbt_dynar_t tokens2;

    loc->type = e_dw_compose;
    loc->location.compose = xbt_dynar_new(sizeof(dw_location_t), NULL);

    while(cursor < xbt_dynar_length(tokens1)){

      tok = xbt_dynar_get_as(tokens1, cursor, char*);
      tokens2 = xbt_str_split(tok, " ");
      tok2 = xbt_dynar_get_as(tokens2, 0, char*);
      
      if(strncmp(tok2, "DW_OP_reg", 9) == 0){
        dw_location_t new_element = xbt_new0(s_dw_location_t, 1);
        new_element->type = e_dw_register;
        new_element->location.reg = atoi(strtok(tok2, "DW_OP_reg"));
        xbt_dynar_push(loc->location.compose, &new_element);     
      }else if(strcmp(tok2, "DW_OP_fbreg:") == 0){
        dw_location_t new_element = xbt_new0(s_dw_location_t, 1);
        new_element->type = e_dw_fbregister_op;
        new_element->location.fbreg_op = atoi(xbt_dynar_get_as(tokens2, xbt_dynar_length(tokens2) - 1, char*));
        xbt_dynar_push(loc->location.compose, &new_element);
      }else if(strncmp(tok2, "DW_OP_breg", 10) == 0){
        dw_location_t new_element = xbt_new0(s_dw_location_t, 1);
        new_element->type = e_dw_bregister_op;
        new_element->location.breg_op.reg = atoi(strtok(tok2, "DW_OP_breg"));
        new_element->location.breg_op.offset = atoi(xbt_dynar_get_as(tokens2, xbt_dynar_length(tokens2) - 1, char*));
        xbt_dynar_push(loc->location.compose, &new_element);
      }else if(strncmp(tok2, "DW_OP_lit", 9) == 0){
        dw_location_t new_element = xbt_new0(s_dw_location_t, 1);
        new_element->type = e_dw_lit;
        new_element->location.lit = atoi(strtok(tok2, "DW_OP_lit"));
        xbt_dynar_push(loc->location.compose, &new_element);
      }else if(strcmp(tok2, "DW_OP_piece:") == 0){
        dw_location_t new_element = xbt_new0(s_dw_location_t, 1);
        new_element->type = e_dw_piece;
        new_element->location.piece = atoi(xbt_dynar_get_as(tokens2, xbt_dynar_length(tokens2) - 1, char*));
        xbt_dynar_push(loc->location.compose, &new_element);
      }else if(strcmp(tok2, "DW_OP_plus_uconst:") == 0){
        dw_location_t new_element = xbt_new0(s_dw_location_t, 1);
        new_element->type = e_dw_plus_uconst;
        new_element->location.plus_uconst = atoi(xbt_dynar_get_as(tokens2, xbt_dynar_length(tokens2) - 1, char *));
        xbt_dynar_push(loc->location.compose, &new_element);
      }else if(strcmp(tok, "DW_OP_abs") == 0 || 
               strcmp(tok, "DW_OP_and") == 0 ||
               strcmp(tok, "DW_OP_div") == 0 ||
               strcmp(tok, "DW_OP_minus") == 0 ||
               strcmp(tok, "DW_OP_mod") == 0 ||
               strcmp(tok, "DW_OP_mul") == 0 ||
               strcmp(tok, "DW_OP_neg") == 0 ||
               strcmp(tok, "DW_OP_not") == 0 ||
               strcmp(tok, "DW_OP_or") == 0 ||
               strcmp(tok, "DW_OP_plus") == 0){               
        dw_location_t new_element = xbt_new0(s_dw_location_t, 1);
        new_element->type = e_dw_arithmetic;
        new_element->location.arithmetic = strdup(strtok(tok2, "DW_OP_"));
        xbt_dynar_push(loc->location.compose, &new_element);
      }else if(strcmp(tok, "DW_OP_stack_value") == 0){
      }else if(strcmp(tok2, "DW_OP_deref_size:") == 0){
        dw_location_t new_element = xbt_new0(s_dw_location_t, 1);
        new_element->type = e_dw_deref;
        new_element->location.deref_size = (unsigned int short) atoi(xbt_dynar_get_as(tokens2, xbt_dynar_length(tokens2) - 1, char*));
        xbt_dynar_push(loc->location.compose, &new_element);
      }else if(strcmp(tok, "DW_OP_deref") == 0){
        dw_location_t new_element = xbt_new0(s_dw_location_t, 1);
        new_element->type = e_dw_deref;
        new_element->location.deref_size = sizeof(void *);
        xbt_dynar_push(loc->location.compose, &new_element);
      }else if(strcmp(tok2, "DW_OP_constu:") == 0){
        dw_location_t new_element = xbt_new0(s_dw_location_t, 1);
        new_element->type = e_dw_uconstant;
        new_element->location.uconstant.bytes = 1;
        new_element->location.uconstant.value = (unsigned long int)(atoi(xbt_dynar_get_as(tokens2, xbt_dynar_length(tokens2) - 1, char*)));
        xbt_dynar_push(loc->location.compose, &new_element);
      }else if(strcmp(tok2, "DW_OP_consts:") == 0){
        dw_location_t new_element = xbt_new0(s_dw_location_t, 1);
        new_element->type = e_dw_sconstant;
        new_element->location.sconstant.bytes = 1;
        new_element->location.sconstant.value = (long int)(atoi(xbt_dynar_get_as(tokens2, xbt_dynar_length(tokens2) - 1, char*)));
        xbt_dynar_push(loc->location.compose, &new_element);
      }else if(strcmp(tok2, "DW_OP_const1u:") == 0 ||
               strcmp(tok2, "DW_OP_const2u:") == 0 ||
               strcmp(tok2, "DW_OP_const4u:") == 0 ||
               strcmp(tok2, "DW_OP_const8u:") == 0){
        dw_location_t new_element = xbt_new0(s_dw_location_t, 1);
        new_element->type = e_dw_uconstant;
        new_element->location.uconstant.bytes = tok2[11] - '0';
        new_element->location.uconstant.value = (unsigned long int)(atoi(xbt_dynar_get_as(tokens2, xbt_dynar_length(tokens2) - 1, char*)));
        xbt_dynar_push(loc->location.compose, &new_element);
      }else if(strcmp(tok, "DW_OP_const1s") == 0 ||
               strcmp(tok, "DW_OP_const2s") == 0 ||
               strcmp(tok, "DW_OP_const4s") == 0 ||
               strcmp(tok, "DW_OP_const8s") == 0){
        dw_location_t new_element = xbt_new0(s_dw_location_t, 1);
        new_element->type = e_dw_sconstant;
        new_element->location.sconstant.bytes = tok2[11] - '0';
        new_element->location.sconstant.value = (long int)(atoi(xbt_dynar_get_as(tokens2, xbt_dynar_length(tokens2) - 1, char*)));
        xbt_dynar_push(loc->location.compose, &new_element);
      }else{
        dw_location_t new_element = xbt_new0(s_dw_location_t, 1);
        new_element->type = e_dw_unsupported;
        xbt_dynar_push(loc->location.compose, &new_element);
      }

      cursor++;
      xbt_dynar_free(&tokens2);

    }
    
    xbt_dynar_free(&tokens1);

    return loc;
    
  }

}

static xbt_dict_t MC_dwarf_get_location_list(const char *elf_file){

  char *command = bprintf("objdump -Wo %s", elf_file);

  FILE *fp = popen(command, "r");

  if(fp == NULL){
    perror("popen for objdump failed");
    xbt_abort();
  }

  int debug = 0; /*Detect if the program has been compiled with -g */

  xbt_dict_t location_list = xbt_dict_new_homogeneous(NULL);
  char *line = NULL, *loc_expr = NULL;
  ssize_t read;
  size_t n = 0;
  int cursor_remove;
  xbt_dynar_t split = NULL;

  while ((read = xbt_getline(&line, &n, fp)) != -1) {

    /* Wipeout the new line character */
    line[read - 1] = '\0';

    xbt_str_trim(line, NULL);
    
    if(n == 0)
      continue;

    if(strlen(line) == 0)
      continue;

    if(debug == 0){

      if(strncmp(line, elf_file, strlen(elf_file)) == 0)
        continue;
      
      if(strncmp(line, "Contents", 8) == 0)
        continue;

      if(strncmp(line, "Offset", 6) == 0){
        debug = 1;
        continue;
      }
    }

    if(debug == 0){
      XBT_INFO("Your program must be compiled with -g");
      xbt_abort();
    }

    xbt_dynar_t loclist = xbt_dynar_new(sizeof(dw_location_entry_t), NULL);

    xbt_str_strip_spaces(line);
    split = xbt_str_split(line, " ");

    while(read != -1 && strcmp("<End", (char *)xbt_dynar_get_as(split, 1, char *)) != 0){
      
      dw_location_entry_t new_entry = xbt_new0(s_dw_location_entry_t, 1);
      new_entry->lowpc = strtoul((char *)xbt_dynar_get_as(split, 1, char *), NULL, 16);
      new_entry->highpc = strtoul((char *)xbt_dynar_get_as(split, 2, char *), NULL, 16);
      
      cursor_remove =0;
      while(cursor_remove < 3){
        xbt_dynar_remove_at(split, 0, NULL);
        cursor_remove++;
      }

      loc_expr = xbt_str_join(split, " ");
      xbt_str_ltrim(loc_expr, "(");
      xbt_str_rtrim(loc_expr, ")");
      new_entry->location = MC_dwarf_get_location(NULL, loc_expr);

      xbt_dynar_push(loclist, &new_entry);

      xbt_dynar_free(&split);
      free(loc_expr);

      read = xbt_getline(&line, &n, fp);
      if(read != -1){
        line[read - 1] = '\0';
        xbt_str_strip_spaces(line);
        split = xbt_str_split(line, " ");
      }

    }


    char *key = bprintf("%d", (int)strtoul((char *)xbt_dynar_get_as(split, 0, char *), NULL, 16));
    xbt_dict_set(location_list, key, loclist, NULL);
    xbt_free(key);
    
    xbt_dynar_free(&split);

  }

  xbt_free(line);
  xbt_free(command);
  pclose(fp);

  return location_list;
}

static dw_frame_t MC_dwarf_get_frame_by_offset(xbt_dict_t all_variables, unsigned long int offset){

  xbt_dict_cursor_t cursor = NULL;
  char *name;
  dw_frame_t res;

  xbt_dict_foreach(all_variables, cursor, name, res) {
    if(offset >= res->start && offset < res->end){
      xbt_dict_cursor_free(&cursor);
      return res;
    }
  }

  xbt_dict_cursor_free(&cursor);
  return NULL;
  
}

static dw_variable_t MC_dwarf_get_variable_by_name(dw_frame_t frame, char *var){

  unsigned int cursor = 0;
  dw_variable_t current_var;

  xbt_dynar_foreach(frame->variables, cursor, current_var){
    if(strcmp(var, current_var->name) == 0)
      return current_var;
  }

  return NULL;
}

static int MC_dwarf_get_variable_index(xbt_dynar_t variables, char* var, void *address){

  if(xbt_dynar_is_empty(variables))
    return 0;

  unsigned int cursor = 0;
  int start = 0;
  int end = xbt_dynar_length(variables) - 1;
  dw_variable_t var_test = NULL;

  while(start <= end){
    cursor = (start + end) / 2;
    var_test = (dw_variable_t)xbt_dynar_get_as(variables, cursor, dw_variable_t);
    if(strcmp(var_test->name, var) < 0){
      start = cursor + 1;
    }else if(strcmp(var_test->name, var) > 0){
      end = cursor - 1;
    }else{
      if(address){ /* global variable */
        if(var_test->address.address == address)
          return -1;
        if(var_test->address.address > address)
          end = cursor - 1;
        else
          start = cursor + 1;
      }else{ /* local variable */
        return -1;
      }
    }
  }

  if(strcmp(var_test->name, var) == 0){
    if(address && var_test->address.address < address)
      return cursor+1;
    else
      return cursor;
  }else if(strcmp(var_test->name, var) < 0)
    return cursor+1;
  else
    return cursor;

}


static void MC_dwarf_get_variables(const char *elf_file, xbt_dict_t location_list, xbt_dict_t *local_variables, xbt_dynar_t *global_variables, xbt_dict_t *types){

  char *command = bprintf("objdump -Wi %s", elf_file);
  
  FILE *fp = popen(command, "r");

  if(fp == NULL)
    perror("popen for objdump failed");

  char *line = NULL, *origin, *abstract_origin, *current_frame = NULL, 
    *subprogram_name = NULL, *subprogram_start = NULL, *subprogram_end = NULL,
    *node_type = NULL, *location_type = NULL, *variable_name = NULL, 
    *loc_expr = NULL, *name = NULL, *end =NULL, *type_origin = NULL, *global_address = NULL, 
    *parent_value = NULL;

  ssize_t read =0;
  size_t n = 0;
  int global_variable = 0, parent = 0, new_frame = 0, new_variable = 1, size = 0, 
    is_pointer = 0, struct_decl = 0, member_end = 0,
    enumeration_size = 0, subrange = 0, union_decl = 0, offset = 0, index = 0;
  
  xbt_dynar_t split = NULL, split2 = NULL;

  xbt_dict_t variables_origin = xbt_dict_new_homogeneous(xbt_free);
  xbt_dict_t subprograms_origin = xbt_dict_new_homogeneous(xbt_free);

  dw_frame_t variable_frame, subroutine_frame = NULL;

  e_dw_type_type type_type = -1;

  read = xbt_getline(&line, &n, fp);

  while (read != -1) {

    /* Wipeout the new line character */
    line[read - 1] = '\0';
  
    if(n == 0 || strlen(line) == 0){
      read = xbt_getline(&line, &n, fp);
      continue;
    }

    xbt_str_ltrim(line, NULL);
    xbt_str_strip_spaces(line);
    
    if(line[0] != '<'){
      read = xbt_getline(&line, &n, fp);
      continue;
    }
    
    xbt_dynar_free(&split);
    split = xbt_str_split(line, " ");

    /* Get node type */
    node_type = xbt_dynar_get_as(split, xbt_dynar_length(split) - 1, char *);

    if(strcmp(node_type, "(DW_TAG_subprogram)") == 0){ /* New frame */

      dw_frame_t frame = NULL;

      strtok(xbt_dynar_get_as(split, 0, char *), "<");
      subprogram_start = xbt_strdup(strtok(NULL, "<"));
      xbt_str_rtrim(subprogram_start, ">:");

      read = xbt_getline(&line, &n, fp);
   
      while(read != -1){

        /* Wipeout the new line character */
        line[read - 1] = '\0';

        if(n == 0 || strlen(line) == 0){
          read = xbt_getline(&line, &n, fp);
          continue;
        }
        
        xbt_dynar_free(&split);
        xbt_str_rtrim(line, NULL);
        xbt_str_strip_spaces(line);
        split = xbt_str_split(line, " ");
          
        node_type = xbt_dynar_get_as(split, 1, char *);

        if(strncmp(node_type, "DW_AT_", 6) != 0)
          break;

        if(strcmp(node_type, "DW_AT_sibling") == 0){

          subprogram_end = xbt_strdup(xbt_dynar_get_as(split, 3, char*));
          xbt_str_ltrim(subprogram_end, "<0x");
          xbt_str_rtrim(subprogram_end, ">");
          
        }else if(strcmp(node_type, "DW_AT_abstract_origin:") == 0){ /* Frame already in dict */
          
          new_frame = 0;
          abstract_origin = xbt_strdup(xbt_dynar_get_as(split, 2, char*));
          xbt_str_ltrim(abstract_origin, "<0x");
          xbt_str_rtrim(abstract_origin, ">");
          subprogram_name = (char *)xbt_dict_get_or_null(subprograms_origin, abstract_origin);
          frame = xbt_dict_get_or_null(*local_variables, subprogram_name); 
          xbt_free(abstract_origin);

        }else if(strcmp(node_type, "DW_AT_name") == 0){

          new_frame = 1;
          xbt_free(current_frame);
          frame = xbt_new0(s_dw_frame_t, 1);
          frame->name = strdup(xbt_dynar_get_as(split, xbt_dynar_length(split) - 1, char *)); 
          frame->variables = xbt_dynar_new(sizeof(dw_variable_t), dw_variable_free_voidp);
          frame->frame_base = xbt_new0(s_dw_location_t, 1); 
          current_frame = strdup(frame->name);

          xbt_dict_set(subprograms_origin, subprogram_start, xbt_strdup(frame->name), NULL);
        
        }else if(strcmp(node_type, "DW_AT_frame_base") == 0){

          location_type = xbt_dynar_get_as(split, xbt_dynar_length(split) - 1, char *);

          if(strcmp(location_type, "list)") == 0){ /* Search location in location list */

            frame->frame_base = MC_dwarf_get_location(location_list, xbt_dynar_get_as(split, 3, char *));
             
          }else{
                
            xbt_str_strip_spaces(line);
            split2 = xbt_str_split(line, "(");
            xbt_dynar_remove_at(split2, 0, NULL);
            loc_expr = xbt_str_join(split2, " ");
            xbt_str_rtrim(loc_expr, ")");
            frame->frame_base = MC_dwarf_get_location(NULL, loc_expr);
            xbt_dynar_free(&split2);
            xbt_free(loc_expr);

          }
 
        }else if(strcmp(node_type, "DW_AT_low_pc") == 0){
          
          if(frame != NULL)
            frame->low_pc = (void *)strtoul(xbt_dynar_get_as(split, 3, char *), NULL, 16);

        }else if(strcmp(node_type, "DW_AT_high_pc") == 0){

          if(frame != NULL)
            frame->high_pc = (void *)strtoul(xbt_dynar_get_as(split, 3, char *), NULL, 16);

        }else if(strcmp(node_type, "DW_AT_MIPS_linkage_name:") == 0){

          xbt_free(frame->name);
          xbt_free(current_frame);
          frame->name = strdup(xbt_dynar_get_as(split, xbt_dynar_length(split) - 1, char *));   
          current_frame = strdup(frame->name);
          xbt_dict_set(subprograms_origin, subprogram_start, xbt_strdup(frame->name), NULL);

        }

        read = xbt_getline(&line, &n, fp);

      }
 
      if(new_frame == 1){
        frame->start = strtoul(subprogram_start, NULL, 16);
        if(subprogram_end != NULL)
          frame->end = strtoul(subprogram_end, NULL, 16);
        xbt_dict_set(*local_variables, frame->name, frame, NULL);
      }

      xbt_free(subprogram_start);
      xbt_free(subprogram_end);
      subprogram_end = NULL;
        

    }else if(strcmp(node_type, "(DW_TAG_variable)") == 0){ /* New variable */

      dw_variable_t var = NULL;
      
      parent_value = strdup(xbt_dynar_get_as(split, 0, char *));
      parent_value = strtok(parent_value,"<");
      xbt_str_rtrim(parent_value, ">");
      parent = atoi(parent_value);
      xbt_free(parent_value);

      if(parent == 1)
        global_variable = 1;
    
      strtok(xbt_dynar_get_as(split, 0, char *), "<");
      origin = xbt_strdup(strtok(NULL, "<"));
      xbt_str_rtrim(origin, ">:");
      
      read = xbt_getline(&line, &n, fp);
      
      while(read != -1){

        /* Wipeout the new line character */
        line[read - 1] = '\0'; 

        if(n == 0 || strlen(line) == 0){
          read = xbt_getline(&line, &n, fp);
          continue;
        }
    
        xbt_dynar_free(&split);
        xbt_str_rtrim(line, NULL);
        xbt_str_strip_spaces(line);
        split = xbt_str_split(line, " ");
  
        node_type = xbt_dynar_get_as(split, 1, char *);

        if(strncmp(node_type, "DW_AT_", 6) != 0)
          break;

        if(strcmp(node_type, "DW_AT_name") == 0){

          var = xbt_new0(s_dw_variable_t, 1);
          var->name = xbt_strdup(xbt_dynar_get_as(split, xbt_dynar_length(split) - 1, char *));
          xbt_dict_set(variables_origin, origin, xbt_strdup(var->name), NULL);
         
        }else if(strcmp(node_type, "DW_AT_abstract_origin:") == 0){

          new_variable = 0;

          abstract_origin = xbt_dynar_get_as(split, 2, char *);
          xbt_str_ltrim(abstract_origin, "<0x");
          xbt_str_rtrim(abstract_origin, ">");
          
          variable_name = (char *)xbt_dict_get_or_null(variables_origin, abstract_origin);
          variable_frame = MC_dwarf_get_frame_by_offset(*local_variables, strtoul(abstract_origin, NULL, 16));
          var = MC_dwarf_get_variable_by_name(variable_frame, variable_name); 

        }else if(strcmp(node_type, "DW_AT_location") == 0){

          if(var != NULL){

            if(!global_variable){

              location_type = xbt_dynar_get_as(split, xbt_dynar_length(split) - 1, char *);

              if(strcmp(location_type, "list)") == 0){ /* Search location in location list */
                var->address.location = MC_dwarf_get_location(location_list, xbt_dynar_get_as(split, 3, char *));
              }else{
                xbt_str_strip_spaces(line);
                split2 = xbt_str_split(line, "(");
                xbt_dynar_remove_at(split2, 0, NULL);
                loc_expr = xbt_str_join(split2, " ");
                xbt_str_rtrim(loc_expr, ")");
                if(strncmp("DW_OP_addr", loc_expr, 10) == 0){
                  global_variable = 1;
                  xbt_dynar_free(&split2);
                  split2 = xbt_str_split(loc_expr, " ");
                  if(strcmp(elf_file, xbt_binary_name) != 0)
                    var->address.address = (char *)start_text_libsimgrid + (long)((void *)strtoul(xbt_dynar_get_as(split2, xbt_dynar_length(split2) - 1, char*), NULL, 16));
                  else
                    var->address.address = (void *)strtoul(xbt_dynar_get_as(split2, xbt_dynar_length(split2) - 1, char*), NULL, 16);
                }else{
                  var->address.location = MC_dwarf_get_location(NULL, loc_expr);
                }
                xbt_dynar_free(&split2);
                xbt_free(loc_expr);
              }
            }else{
              global_address = xbt_strdup(xbt_dynar_get_as(split, xbt_dynar_length(split) - 1, char *));
              xbt_str_rtrim(global_address, ")");
              if(strcmp(elf_file, xbt_binary_name) != 0)
                var->address.address = (char *)start_text_libsimgrid + (long)((void *)strtoul(global_address, NULL, 16));
              else
                var->address.address = (void *)strtoul(global_address, NULL, 16);
              xbt_free(global_address);
              global_address = NULL;
            }

          }
                   
        }else if(strcmp(node_type, "DW_AT_type") == 0){
          
          type_origin = xbt_strdup(xbt_dynar_get_as(split, 3, char *));
          xbt_str_ltrim(type_origin, "<0x");
          xbt_str_rtrim(type_origin, ">");
        
        }else if(strcmp(node_type, "DW_AT_declaration") == 0){

          new_variable = 0;
          if(new_variable){
            dw_variable_free(var);
            var = NULL;
          }
        
        }else if(strcmp(node_type, "DW_AT_artificial") == 0){
          
          new_variable = 0;
          if(new_variable){
            dw_variable_free(var);
            var = NULL;
          }
        
        }

        read = xbt_getline(&line, &n, fp);
 
      }

      if(new_variable == 1){
        
        if(!global_variable){
          variable_frame = xbt_dict_get_or_null(*local_variables, current_frame);
          var->type_origin = strdup(type_origin);
          var->global = 0;
          index = MC_dwarf_get_variable_index(variable_frame->variables, var->name, NULL);
          if(index != -1)
            xbt_dynar_insert_at(variable_frame->variables, index, &var);
        }else{
          var->type_origin = strdup(type_origin);
          var->global = 1;
          index = MC_dwarf_get_variable_index(*global_variables, var->name, var->address.address);
          if(index != -1)
            xbt_dynar_insert_at(*global_variables, index, &var); 
        }

         xbt_free(type_origin);
         type_origin = NULL;
      }

      global_variable = 0;
      new_variable = 1;

    }else if(strcmp(node_type, "(DW_TAG_inlined_subroutine)") == 0){

      read = xbt_getline(&line, &n, fp);

      while(read != -1){

        /* Wipeout the new line character */
        line[read - 1] = '\0'; 

        if(n == 0 || strlen(line) == 0){
          read = xbt_getline(&line, &n, fp);
          continue;
        }

        xbt_dynar_free(&split);
        xbt_str_rtrim(line, NULL);
        xbt_str_strip_spaces(line);
        split = xbt_str_split(line, " ");
        
        if(strncmp(xbt_dynar_get_as(split, 1, char *), "DW_AT_", 6) != 0)
          break;
          
        node_type = xbt_dynar_get_as(split, 1, char *);

        if(strcmp(node_type, "DW_AT_abstract_origin:") == 0){

          origin = xbt_dynar_get_as(split, 2, char *);
          xbt_str_ltrim(origin, "<0x");
          xbt_str_rtrim(origin, ">");
          
          subprogram_name = (char *)xbt_dict_get_or_null(subprograms_origin, origin);
          subroutine_frame = xbt_dict_get_or_null(*local_variables, subprogram_name);
        
        }else if(strcmp(node_type, "DW_AT_low_pc") == 0){

          subroutine_frame->low_pc = (void *)strtoul(xbt_dynar_get_as(split, 3, char *), NULL, 16);

        }else if(strcmp(node_type, "DW_AT_high_pc") == 0){

          subroutine_frame->high_pc = (void *)strtoul(xbt_dynar_get_as(split, 3, char *), NULL, 16);
        }

        read = xbt_getline(&line, &n, fp);
      
      }

    }else if(strcmp(node_type, "(DW_TAG_base_type)") == 0 
             || strcmp(node_type, "(DW_TAG_enumeration_type)") == 0
             || strcmp(node_type, "(DW_TAG_enumerator)") == 0
             || strcmp(node_type, "(DW_TAG_typedef)") == 0
             || strcmp(node_type, "(DW_TAG_const_type)") == 0
             || strcmp(node_type, "(DW_TAG_subroutine_type)") == 0
             || strcmp(node_type, "(DW_TAG_volatile_type)") == 0
             || (is_pointer = !strcmp(node_type, "(DW_TAG_pointer_type)"))){

      if(strcmp(node_type, "(DW_TAG_base_type)") == 0)
        type_type = e_dw_base_type;
      else if(strcmp(node_type, "(DW_TAG_enumeration_type)") == 0)
        type_type = e_dw_enumeration_type;
      else if(strcmp(node_type, "(DW_TAG_enumerator)") == 0)
        type_type = e_dw_enumerator;
      else if(strcmp(node_type, "(DW_TAG_typedef)") == 0)
        type_type = e_dw_typedef;
      else if(strcmp(node_type, "(DW_TAG_const_type)") == 0)
        type_type = e_dw_const_type;
      else if(strcmp(node_type, "(DW_TAG_pointer_type)") == 0)
        type_type = e_dw_pointer_type;
      else if(strcmp(node_type, "(DW_TAG_subroutine_type)") == 0)
        type_type = e_dw_subroutine_type;
      else if(strcmp(node_type, "(DW_TAG_volatile_type)") == 0)
        type_type = e_dw_volatile_type;

      strtok(xbt_dynar_get_as(split, 0, char *), "<");
      origin = strdup(strtok(NULL, "<"));
      xbt_str_rtrim(origin, ">:");
      
      read = xbt_getline(&line, &n, fp);
      
      while(read != -1){
        
         /* Wipeout the new line character */
        line[read - 1] = '\0'; 

        if(n == 0 || strlen(line) == 0){
          read = xbt_getline(&line, &n, fp);
          continue;
        }

        xbt_dynar_free(&split);
        xbt_str_rtrim(line, NULL);
        xbt_str_strip_spaces(line);
        split = xbt_str_split(line, " ");
        
        if(strncmp(xbt_dynar_get_as(split, 1, char *), "DW_AT_", 6) != 0)
          break;

        node_type = xbt_dynar_get_as(split, 1, char *);

        if(strcmp(node_type, "DW_AT_byte_size") == 0){
          size = strtol(xbt_dynar_get_as(split, xbt_dynar_length(split) - 1, char*), NULL, 10);
          if(type_type == e_dw_enumeration_type)
            enumeration_size = size;
        }else if(strcmp(node_type, "DW_AT_name") == 0){
          end = xbt_str_join(split, " ");
          xbt_dynar_free(&split);
          split = xbt_str_split(end, "):");
          xbt_str_ltrim(xbt_dynar_get_as(split, xbt_dynar_length(split) - 1, char*), NULL);
          name = xbt_strdup(xbt_dynar_get_as(split, xbt_dynar_length(split) - 1, char*));
        }else if(strcmp(node_type, "DW_AT_type") == 0){
          type_origin = xbt_strdup(xbt_dynar_get_as(split, 3, char *));
          xbt_str_ltrim(type_origin, "<0x");
          xbt_str_rtrim(type_origin, ">");
        }
        
        read = xbt_getline(&line, &n, fp);
      }

      dw_type_t type = xbt_new0(s_dw_type_t, 1);
      type->type = type_type;
      if(name)
        type->name = xbt_strdup(name);
      else
        type->name = xbt_strdup("undefined");
      type->is_pointer_type = is_pointer;
      type->id = (void *)strtoul(origin, NULL, 16);
      if(type_origin)
        type->dw_type_id = xbt_strdup(type_origin);
      if(type_type == e_dw_enumerator)
        type->size = enumeration_size;
      else
        type->size = size;
      type->members = NULL;

      xbt_dict_set(*types, origin, type, NULL); 

      xbt_free(name);
      name = NULL;
      xbt_free(type_origin);
      type_origin = NULL;
      xbt_free(end);
      end = NULL;

      is_pointer = 0;
      size = 0;
      xbt_free(origin);

    }else if(strcmp(node_type, "(DW_TAG_structure_type)") == 0 || strcmp(node_type, "(DW_TAG_union_type)") == 0){
      
      if(strcmp(node_type, "(DW_TAG_structure_type)") == 0)
        struct_decl = 1;
      else
        union_decl = 1;

      strtok(xbt_dynar_get_as(split, 0, char *), "<");
      origin = strdup(strtok(NULL, "<"));
      xbt_str_rtrim(origin, ">:");
      
      read = xbt_getline(&line, &n, fp);

      dw_type_t type = NULL;

      while(read != -1){
      
        while(read != -1){
        
          /* Wipeout the new line character */
          line[read - 1] = '\0'; 

          if(n == 0 || strlen(line) == 0){
            read = xbt_getline(&line, &n, fp);
            continue;
          }

          xbt_dynar_free(&split);
          xbt_str_rtrim(line, NULL);
          xbt_str_strip_spaces(line);
          split = xbt_str_split(line, " ");
        
          node_type = xbt_dynar_get_as(split, 1, char *);

          if(strncmp(node_type, "DW_AT_", 6) != 0){
            member_end = 1;
            break;
          }

          if(strcmp(node_type, "DW_AT_byte_size") == 0){
            size = strtol(xbt_dynar_get_as(split, xbt_dynar_length(split) - 1, char*), NULL, 10);
          }else if(strcmp(node_type, "DW_AT_name") == 0){
            xbt_free(end);
            end = xbt_str_join(split, " ");
            xbt_dynar_free(&split);
            split = xbt_str_split(end, "):");
            xbt_str_ltrim(xbt_dynar_get_as(split, xbt_dynar_length(split) - 1, char*), NULL);
            name = xbt_strdup(xbt_dynar_get_as(split, xbt_dynar_length(split) - 1, char*));
          }else if(strcmp(node_type, "DW_AT_type") == 0){
            type_origin = xbt_strdup(xbt_dynar_get_as(split, 3, char *));
            xbt_str_ltrim(type_origin, "<0x");
            xbt_str_rtrim(type_origin, ">");
          }else if(strcmp(node_type, "DW_AT_data_member_location:") == 0){
            xbt_free(end);
            end = xbt_str_join(split, " ");
            xbt_dynar_free(&split);
            split = xbt_str_split(end, "DW_OP_plus_uconst:");
            xbt_str_ltrim(xbt_dynar_get_as(split, xbt_dynar_length(split) - 1, char *), NULL);
            xbt_str_rtrim(xbt_dynar_get_as(split, xbt_dynar_length(split) - 1, char *), ")");
            offset = strtol(xbt_dynar_get_as(split, xbt_dynar_length(split) - 1, char*), NULL, 10);
          }

          read = xbt_getline(&line, &n, fp);
          
        }

        if(member_end && type){         
          member_end = 0;
          
          dw_type_t member_type = xbt_new0(s_dw_type_t, 1);
          member_type->name = xbt_strdup(name);
          member_type->size = size;
          member_type->is_pointer_type = is_pointer;
          member_type->id = (void *)strtoul(origin, NULL, 16);
          member_type->offset = offset;
          if(type_origin)
            member_type->dw_type_id = xbt_strdup(type_origin);

          xbt_dynar_push(type->members, &member_type);

          xbt_free(name);
          name = NULL;
          xbt_free(end);
          end = NULL;
          xbt_free(type_origin);
          type_origin = NULL;
          size = 0;
          offset = 0;

          xbt_free(origin);
          origin = NULL;
          strtok(xbt_dynar_get_as(split, 0, char *), "<");
          origin = strdup(strtok(NULL, "<"));
          xbt_str_rtrim(origin, ">:");

        }

        if(struct_decl || union_decl){
          type = xbt_new0(s_dw_type_t, 1);
          if(struct_decl)
            type->type = e_dw_structure_type;
          else
            type->type = e_dw_union_type;
          type->name = xbt_strdup(name);
          type->size = size;
          type->is_pointer_type = is_pointer;
          type->id = (void *)strtoul(origin, NULL, 16);
          if(type_origin)
            type->dw_type_id = xbt_strdup(type_origin);
          type->members = xbt_dynar_new(sizeof(dw_type_t), dw_type_free_voidp);
          
          xbt_dict_set(*types, origin, type, NULL); 
          
          xbt_free(name);
          name = NULL;
          xbt_free(end);
          end = NULL;
          xbt_free(type_origin);
          type_origin = NULL;
          size = 0;
          struct_decl = 0;
          union_decl = 0;

        }

        if(strcmp(xbt_dynar_get_as(split, xbt_dynar_length(split) - 1, char *), "(DW_TAG_member)") != 0)
          break;  

        read = xbt_getline(&line, &n, fp);
    
      }

      xbt_free(origin);
      origin = NULL;

    }else if(strcmp(node_type, "(DW_TAG_array_type)") == 0){
      
      strtok(xbt_dynar_get_as(split, 0, char *), "<");
      origin = strdup(strtok(NULL, "<"));
      xbt_str_rtrim(origin, ">:");
      
      read = xbt_getline(&line, &n, fp);

      dw_type_t type = NULL;

      while(read != -1){
      
        while(read != -1){
        
          /* Wipeout the new line character */
          line[read - 1] = '\0'; 

          if(n == 0 || strlen(line) == 0){
            read = xbt_getline(&line, &n, fp);
            continue;
          }

          xbt_dynar_free(&split);
          xbt_str_rtrim(line, NULL);
          xbt_str_strip_spaces(line);
          split = xbt_str_split(line, " ");
        
          node_type = xbt_dynar_get_as(split, 1, char *);

          if(strncmp(node_type, "DW_AT_", 6) != 0)
            break;

          if(strcmp(node_type, "DW_AT_upper_bound") == 0){
            size = strtol(xbt_dynar_get_as(split, xbt_dynar_length(split) - 1, char*), NULL, 10);
          }else if(strcmp(node_type, "DW_AT_name") == 0){
            end = xbt_str_join(split, " ");
            xbt_dynar_free(&split);
            split = xbt_str_split(end, "):");
            xbt_str_ltrim(xbt_dynar_get_as(split, xbt_dynar_length(split) - 1, char*), NULL);
            name = xbt_strdup(xbt_dynar_get_as(split, xbt_dynar_length(split) - 1, char*));
          }else if(strcmp(node_type, "DW_AT_type") == 0){
            type_origin = xbt_strdup(xbt_dynar_get_as(split, 3, char *));
            xbt_str_ltrim(type_origin, "<0x");
            xbt_str_rtrim(type_origin, ">");
          }

          read = xbt_getline(&line, &n, fp);
          
        }

        if(strcmp(xbt_dynar_get_as(split, xbt_dynar_length(split) - 1, char *), "(DW_TAG_subrange_type)") == 0){
          subrange = 1;         
        }

        if(subrange && type){         
          type->size = size;
      
          xbt_free(name);
          name = NULL;
          xbt_free(end);
          end = NULL;
          xbt_free(type_origin);
          type_origin = NULL;
          size = 0;

          xbt_free(origin);
          origin = NULL;
          strtok(xbt_dynar_get_as(split, 0, char *), "<");
          origin = strdup(strtok(NULL, "<"));
          xbt_str_rtrim(origin, ">:");

        }else {
          
          type = xbt_new0(s_dw_type_t, 1);
          type->type = e_dw_array_type;
          type->name = xbt_strdup(name);
          type->is_pointer_type = is_pointer;
          type->id = (void *)strtoul(origin, NULL, 16);
          if(type_origin)
            type->dw_type_id = xbt_strdup(type_origin);
          type->members = NULL;
          
          xbt_dict_set(*types, origin, type, NULL); 
          
          xbt_free(name);
          name = NULL;
          xbt_free(end);
          end = NULL;
          xbt_free(type_origin);
          type_origin = NULL;
          size = 0;
        }

        if(strcmp(xbt_dynar_get_as(split, xbt_dynar_length(split) - 1, char *), "(DW_TAG_subrange_type)") != 0)
          break;  

        read = xbt_getline(&line, &n, fp);
    
      }

      xbt_free(origin);
      origin = NULL;

    }else{

      read = xbt_getline(&line, &n, fp);

    }

  }
  
  xbt_dynar_free(&split);
  xbt_dict_free(&variables_origin);
  xbt_dict_free(&subprograms_origin);
  xbt_free(line);
  xbt_free(command);
  pclose(fp);
  
}


/*******************************  Ignore mechanism *******************************/
/*********************************************************************************/

xbt_dynar_t mc_checkpoint_ignore;

typedef struct s_mc_stack_ignore_variable{
  char *var_name;
  char *frame;
}s_mc_stack_ignore_variable_t, *mc_stack_ignore_variable_t;

typedef struct s_mc_data_bss_ignore_variable{
  char *name;
}s_mc_data_bss_ignore_variable_t, *mc_data_bss_ignore_variable_t;

/**************************** Free functions ******************************/

static void stack_ignore_variable_free(mc_stack_ignore_variable_t v){
  xbt_free(v->var_name);
  xbt_free(v->frame);
  xbt_free(v);
}

static void stack_ignore_variable_free_voidp(void *v){
  stack_ignore_variable_free((mc_stack_ignore_variable_t) * (void **) v);
}

void heap_ignore_region_free(mc_heap_ignore_region_t r){
  xbt_free(r);
}

void heap_ignore_region_free_voidp(void *r){
  heap_ignore_region_free((mc_heap_ignore_region_t) * (void **) r);
}

static void data_bss_ignore_variable_free(mc_data_bss_ignore_variable_t v){
  xbt_free(v->name);
  xbt_free(v);
}

static void data_bss_ignore_variable_free_voidp(void *v){
  data_bss_ignore_variable_free((mc_data_bss_ignore_variable_t) * (void **) v);
}

static void checkpoint_ignore_region_free(mc_checkpoint_ignore_region_t r){
  xbt_free(r);
}

static void checkpoint_ignore_region_free_voidp(void *r){
  checkpoint_ignore_region_free((mc_checkpoint_ignore_region_t) * (void **) r);
}

/***********************************************************************/

void MC_ignore_heap(void *address, size_t size){

  int raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  MC_SET_RAW_MEM;

  mc_heap_ignore_region_t region = NULL;
  region = xbt_new0(s_mc_heap_ignore_region_t, 1);
  region->address = address;
  region->size = size;
  
  region->block = ((char*)address - (char*)((xbt_mheap_t)std_heap)->heapbase) / BLOCKSIZE + 1;
  
  if(((xbt_mheap_t)std_heap)->heapinfo[region->block].type == 0){
    region->fragment = -1;
    ((xbt_mheap_t)std_heap)->heapinfo[region->block].busy_block.ignore++;
  }else{
    region->fragment = ((uintptr_t) (ADDR2UINT (address) % (BLOCKSIZE))) >> ((xbt_mheap_t)std_heap)->heapinfo[region->block].type;
    ((xbt_mheap_t)std_heap)->heapinfo[region->block].busy_frag.ignore[region->fragment]++;
  }
  
  if(mc_heap_comparison_ignore == NULL){
    mc_heap_comparison_ignore = xbt_dynar_new(sizeof(mc_heap_ignore_region_t), heap_ignore_region_free_voidp);
    xbt_dynar_push(mc_heap_comparison_ignore, &region);
    if(!raw_mem_set)
      MC_UNSET_RAW_MEM;
    return;
  }

  unsigned int cursor = 0;
  mc_heap_ignore_region_t current_region = NULL;
  int start = 0;
  int end = xbt_dynar_length(mc_heap_comparison_ignore) - 1;
  
  while(start <= end){
    cursor = (start + end) / 2;
    current_region = (mc_heap_ignore_region_t)xbt_dynar_get_as(mc_heap_comparison_ignore, cursor, mc_heap_ignore_region_t);
    if(current_region->address == address){
      heap_ignore_region_free(region);
      if(!raw_mem_set)
        MC_UNSET_RAW_MEM;
      return;
    }else if(current_region->address < address){
      start = cursor + 1;
    }else{
      end = cursor - 1;
    }   
  }

  if(current_region->address < address)
    xbt_dynar_insert_at(mc_heap_comparison_ignore, cursor + 1, &region);
  else
    xbt_dynar_insert_at(mc_heap_comparison_ignore, cursor, &region);

  if(!raw_mem_set)
    MC_UNSET_RAW_MEM;
}

void MC_remove_ignore_heap(void *address, size_t size){

  int raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  MC_SET_RAW_MEM;

  unsigned int cursor = 0;
  int start = 0;
  int end = xbt_dynar_length(mc_heap_comparison_ignore) - 1;
  mc_heap_ignore_region_t region;
  int ignore_found = 0;

  while(start <= end){
    cursor = (start + end) / 2;
    region = (mc_heap_ignore_region_t)xbt_dynar_get_as(mc_heap_comparison_ignore, cursor, mc_heap_ignore_region_t);
    if(region->address == address){
      ignore_found = 1;
      break;
    }else if(region->address < address){
      start = cursor + 1;
    }else{
      if((char * )region->address <= ((char *)address + size)){
        ignore_found = 1;
        break;
      }else{
        end = cursor - 1;   
      }
    }
  }
  
  if(ignore_found == 1){
    xbt_dynar_remove_at(mc_heap_comparison_ignore, cursor, NULL);
    MC_remove_ignore_heap(address, size);
  }

  if(!raw_mem_set)
    MC_UNSET_RAW_MEM;

}

void MC_ignore_global_variable(const char *name){

  int raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  MC_SET_RAW_MEM;

  if(mc_global_variables_libsimgrid){

    unsigned int cursor = 0;
    dw_variable_t current_var;
    int start = 0;
    int end = xbt_dynar_length(mc_global_variables_libsimgrid) - 1;

    while(start <= end){
      cursor = (start + end) /2;
      current_var = (dw_variable_t)xbt_dynar_get_as(mc_global_variables_libsimgrid, cursor, dw_variable_t);
      if(strcmp(current_var->name, name) == 0){
        xbt_dynar_remove_at(mc_global_variables_libsimgrid, cursor, NULL);
        start = 0;
        end = xbt_dynar_length(mc_global_variables_libsimgrid) - 1;
      }else if(strcmp(current_var->name, name) < 0){
        start = cursor + 1;
      }else{
        end = cursor - 1;
      } 
    }
   
  }else{

    if(mc_data_bss_comparison_ignore == NULL)
      mc_data_bss_comparison_ignore = xbt_dynar_new(sizeof(mc_data_bss_ignore_variable_t), data_bss_ignore_variable_free_voidp);

    mc_data_bss_ignore_variable_t var = NULL;
    var = xbt_new0(s_mc_data_bss_ignore_variable_t, 1);
    var->name = strdup(name);

    if(xbt_dynar_is_empty(mc_data_bss_comparison_ignore)){

      xbt_dynar_insert_at(mc_data_bss_comparison_ignore, 0, &var);

    }else{
    
      unsigned int cursor = 0;
      int start = 0;
      int end = xbt_dynar_length(mc_data_bss_comparison_ignore) - 1;
      mc_data_bss_ignore_variable_t current_var = NULL;

      while(start <= end){
        cursor = (start + end) / 2;
        current_var = (mc_data_bss_ignore_variable_t)xbt_dynar_get_as(mc_data_bss_comparison_ignore, cursor, mc_data_bss_ignore_variable_t);
        if(strcmp(current_var->name, name) == 0){
          data_bss_ignore_variable_free(var);
          if(!raw_mem_set)
            MC_UNSET_RAW_MEM;
          return;
        }else if(strcmp(current_var->name, name) < 0){
          start = cursor + 1;
        }else{
          end = cursor - 1;
        }
      }

      if(strcmp(current_var->name, name) < 0)
        xbt_dynar_insert_at(mc_data_bss_comparison_ignore, cursor + 1, &var);
      else
        xbt_dynar_insert_at(mc_data_bss_comparison_ignore, cursor, &var);

    }
  }

  if(!raw_mem_set)
    MC_UNSET_RAW_MEM;
}

void MC_ignore_local_variable(const char *var_name, const char *frame_name){
  
  int raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  MC_SET_RAW_MEM;

  if(mc_local_variables_libsimgrid){
    unsigned int cursor = 0;
    dw_variable_t current_var;
    int start, end;
    if(strcmp(frame_name, "*") == 0){ /* Remove variable in all frames */
      xbt_dict_cursor_t dict_cursor;
      char *current_frame_name;
      dw_frame_t frame;
      xbt_dict_foreach(mc_local_variables_libsimgrid, dict_cursor, current_frame_name, frame){
        start = 0;
        end = xbt_dynar_length(frame->variables) - 1;
        while(start <= end){
          cursor = (start + end) / 2;
          current_var = (dw_variable_t)xbt_dynar_get_as(frame->variables, cursor, dw_variable_t); 
          if(strcmp(current_var->name, var_name) == 0){
            xbt_dynar_remove_at(frame->variables, cursor, NULL);
            start = 0;
            end = xbt_dynar_length(frame->variables) - 1;
          }else if(strcmp(current_var->name, var_name) < 0){
            start = cursor + 1;
          }else{
            end = cursor - 1;
          } 
        }
      }
       xbt_dict_foreach(mc_local_variables_binary, dict_cursor, current_frame_name, frame){
        start = 0;
        end = xbt_dynar_length(frame->variables) - 1;
        while(start <= end){
          cursor = (start + end) / 2;
          current_var = (dw_variable_t)xbt_dynar_get_as(frame->variables, cursor, dw_variable_t); 
          if(strcmp(current_var->name, var_name) == 0){
            xbt_dynar_remove_at(frame->variables, cursor, NULL);
            start = 0;
            end = xbt_dynar_length(frame->variables) - 1;
          }else if(strcmp(current_var->name, var_name) < 0){
            start = cursor + 1;
          }else{
            end = cursor - 1;
          } 
        }
      }
    }else{
      xbt_dynar_t variables_list = ((dw_frame_t)xbt_dict_get_or_null(mc_local_variables_libsimgrid, frame_name))->variables;
      start = 0;
      end = xbt_dynar_length(variables_list) - 1;
      while(start <= end){
        cursor = (start + end) / 2;
        current_var = (dw_variable_t)xbt_dynar_get_as(variables_list, cursor, dw_variable_t);
        if(strcmp(current_var->name, var_name) == 0){
          xbt_dynar_remove_at(variables_list, cursor, NULL);
          start = 0;
          end = xbt_dynar_length(variables_list) - 1;
        }else if(strcmp(current_var->name, var_name) < 0){
          start = cursor + 1;
        }else{
          end = cursor - 1;
        } 
      }
    } 
  }else{

    if(mc_stack_comparison_ignore == NULL)
      mc_stack_comparison_ignore = xbt_dynar_new(sizeof(mc_stack_ignore_variable_t), stack_ignore_variable_free_voidp);
  
    mc_stack_ignore_variable_t var = NULL;
    var = xbt_new0(s_mc_stack_ignore_variable_t, 1);
    var->var_name = strdup(var_name);
    var->frame = strdup(frame_name);
  
    if(xbt_dynar_is_empty(mc_stack_comparison_ignore)){

      xbt_dynar_insert_at(mc_stack_comparison_ignore, 0, &var);

    }else{
    
      unsigned int cursor = 0;
      int start = 0;
      int end = xbt_dynar_length(mc_stack_comparison_ignore) - 1;
      mc_stack_ignore_variable_t current_var = NULL;

      while(start <= end){
        cursor = (start + end) / 2;
        current_var = (mc_stack_ignore_variable_t)xbt_dynar_get_as(mc_stack_comparison_ignore, cursor, mc_stack_ignore_variable_t);
        if(strcmp(current_var->frame, frame_name) == 0){
          if(strcmp(current_var->var_name, var_name) == 0){
            stack_ignore_variable_free(var);
            if(!raw_mem_set)
              MC_UNSET_RAW_MEM;
            return;
          }else if(strcmp(current_var->var_name, var_name) < 0){
            start = cursor + 1;
          }else{
            end = cursor - 1;
          }
        }else if(strcmp(current_var->frame, frame_name) < 0){
          start = cursor + 1;
        }else{
          end = cursor - 1;
        }
      }

      if(strcmp(current_var->frame, frame_name) == 0){
        if(strcmp(current_var->var_name, var_name) < 0){
          xbt_dynar_insert_at(mc_stack_comparison_ignore, cursor + 1, &var);
        }else{
          xbt_dynar_insert_at(mc_stack_comparison_ignore, cursor, &var);
        }
      }else if(strcmp(current_var->frame, frame_name) < 0){
        xbt_dynar_insert_at(mc_stack_comparison_ignore, cursor + 1, &var);
      }else{
        xbt_dynar_insert_at(mc_stack_comparison_ignore, cursor, &var);
      }
    }
  }

  if(!raw_mem_set)
    MC_UNSET_RAW_MEM;

}

void MC_new_stack_area(void *stack, char *name, void* context, size_t size){

  int raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  MC_SET_RAW_MEM;

  if(stacks_areas == NULL)
    stacks_areas = xbt_dynar_new(sizeof(stack_region_t), NULL);
  
  stack_region_t region = NULL;
  region = xbt_new0(s_stack_region_t, 1);
  region->address = stack;
  region->process_name = strdup(name);
  region->context = context;
  region->size = size;
  region->block = ((char*)stack - (char*)((xbt_mheap_t)std_heap)->heapbase) / BLOCKSIZE + 1;
  xbt_dynar_push(stacks_areas, &region);

  if(!raw_mem_set)
    MC_UNSET_RAW_MEM;
}

void MC_ignore(void *addr, size_t size){

  int raw_mem_set= (mmalloc_get_current_heap() == raw_heap);

  MC_SET_RAW_MEM;

  if(mc_checkpoint_ignore == NULL)
    mc_checkpoint_ignore = xbt_dynar_new(sizeof(mc_checkpoint_ignore_region_t), checkpoint_ignore_region_free_voidp);

  mc_checkpoint_ignore_region_t region = xbt_new0(s_mc_checkpoint_ignore_region_t, 1);
  region->addr = addr;
  region->size = size;

  if(xbt_dynar_is_empty(mc_checkpoint_ignore)){
    xbt_dynar_push(mc_checkpoint_ignore, &region);
  }else{
     
    unsigned int cursor = 0;
    int start = 0;
    int end = xbt_dynar_length(mc_checkpoint_ignore) -1;
    mc_checkpoint_ignore_region_t current_region = NULL;

    while(start <= end){
      cursor = (start + end) / 2;
      current_region = (mc_checkpoint_ignore_region_t)xbt_dynar_get_as(mc_checkpoint_ignore, cursor, mc_checkpoint_ignore_region_t);
      if(current_region->addr == addr){
        if(current_region->size == size){
          checkpoint_ignore_region_free(region);
          if(!raw_mem_set)
            MC_UNSET_RAW_MEM;
          return;
        }else if(current_region->size < size){
          start = cursor + 1;
        }else{
          end = cursor - 1;
        }
      }else if(current_region->addr < addr){
          start = cursor + 1;
      }else{
        end = cursor - 1;
      }
    }

     if(current_region->addr == addr){
       if(current_region->size < size){
        xbt_dynar_insert_at(mc_checkpoint_ignore, cursor + 1, &region);
      }else{
        xbt_dynar_insert_at(mc_checkpoint_ignore, cursor, &region);
      }
    }else if(current_region->addr < addr){
       xbt_dynar_insert_at(mc_checkpoint_ignore, cursor + 1, &region);
    }else{
       xbt_dynar_insert_at(mc_checkpoint_ignore, cursor, &region);
    }
  }

  if(!raw_mem_set)
    MC_UNSET_RAW_MEM;
}

/*******************************  Initialisation of MC *******************************/
/*********************************************************************************/

static void MC_dump_ignored_local_variables(void){

  if(mc_stack_comparison_ignore == NULL || xbt_dynar_is_empty(mc_stack_comparison_ignore))
    return;

  unsigned int cursor = 0;
  mc_stack_ignore_variable_t current_var;

  xbt_dynar_foreach(mc_stack_comparison_ignore, cursor, current_var){
    MC_ignore_local_variable(current_var->var_name, current_var->frame);
  }

  xbt_dynar_free(&mc_stack_comparison_ignore);
  mc_stack_comparison_ignore = NULL;
 
}

static void MC_dump_ignored_global_variables(void){

  if(mc_data_bss_comparison_ignore == NULL || xbt_dynar_is_empty(mc_data_bss_comparison_ignore))
    return;

  unsigned int cursor = 0;
  mc_data_bss_ignore_variable_t current_var;

  xbt_dynar_foreach(mc_data_bss_comparison_ignore, cursor, current_var){
    MC_ignore_global_variable(current_var->name);
  } 

  xbt_dynar_free(&mc_data_bss_comparison_ignore);
  mc_data_bss_comparison_ignore = NULL;

}

void MC_init(){

  int raw_mem_set = (mmalloc_get_current_heap() == raw_heap);
  
  compare = 0;

  /* Initialize the data structures that must be persistent across every
     iteration of the model-checker (in RAW memory) */

  MC_SET_RAW_MEM;

  MC_init_memory_map_info();
  
  mc_local_variables_libsimgrid = xbt_dict_new_homogeneous(NULL);
  mc_local_variables_binary = xbt_dict_new_homogeneous(NULL);
  mc_global_variables_libsimgrid = xbt_dynar_new(sizeof(dw_variable_t), dw_variable_free_voidp);
  mc_global_variables_binary = xbt_dynar_new(sizeof(dw_variable_t), dw_variable_free_voidp);
  mc_variables_type_libsimgrid = xbt_dict_new_homogeneous(NULL);
  mc_variables_type_binary = xbt_dict_new_homogeneous(NULL);

  XBT_INFO("Get debug information ...");

  /* Get local variables in binary for state equality detection */
  xbt_dict_t binary_location_list = MC_dwarf_get_location_list(xbt_binary_name);
  MC_dwarf_get_variables(xbt_binary_name, binary_location_list, &mc_local_variables_binary, &mc_global_variables_binary, &mc_variables_type_binary);

  /* Get local variables in libsimgrid for state equality detection */
  xbt_dict_t libsimgrid_location_list = MC_dwarf_get_location_list(libsimgrid_path);
  MC_dwarf_get_variables(libsimgrid_path, libsimgrid_location_list, &mc_local_variables_libsimgrid, &mc_global_variables_libsimgrid, &mc_variables_type_libsimgrid);

  xbt_dict_free(&libsimgrid_location_list);
  xbt_dict_free(&binary_location_list);

  XBT_INFO("Get debug information done !");

  /* Remove variables ignored before getting list of variables */
  MC_dump_ignored_local_variables();
  MC_dump_ignored_global_variables();
  
  /* Get .plt section (start and end addresses) for data libsimgrid and data program comparison */
  MC_get_libsimgrid_plt_section();
  MC_get_binary_plt_section();

   /* Init parmap */
  parmap = xbt_parmap_mc_new(xbt_os_get_numcores(), XBT_PARMAP_DEFAULT);

  MC_UNSET_RAW_MEM;

   /* Ignore some variables from xbt/ex.h used by exception e for stacks comparison */
  MC_ignore_local_variable("e", "*");
  MC_ignore_local_variable("__ex_cleanup", "*");
  MC_ignore_local_variable("__ex_mctx_en", "*");
  MC_ignore_local_variable("__ex_mctx_me", "*");
  MC_ignore_local_variable("__xbt_ex_ctx_ptr", "*");
  MC_ignore_local_variable("_log_ev", "*");
  MC_ignore_local_variable("_throw_ctx", "*");
  MC_ignore_local_variable("ctx", "*");

  MC_ignore_local_variable("next_context", "smx_ctx_sysv_suspend_serial");
  MC_ignore_local_variable("i", "smx_ctx_sysv_suspend_serial");

  /* Ignore local variable about time used for tracing */
  MC_ignore_local_variable("start_time", "*"); 

  MC_ignore_global_variable("mc_comp_times");
  MC_ignore_global_variable("mc_snapshot_comparison_time"); 
  MC_ignore_global_variable("mc_time");
  MC_ignore_global_variable("smpi_current_rank");
  MC_ignore_global_variable("counter"); /* Static variable used for tracing */
  MC_ignore_global_variable("maestro_stack_start");
  MC_ignore_global_variable("maestro_stack_end");
 
  MC_ignore_heap(&(simix_global->process_to_run), sizeof(simix_global->process_to_run));
  MC_ignore_heap(&(simix_global->process_that_ran), sizeof(simix_global->process_that_ran));
  smx_process_t process;
  xbt_swag_foreach(process, simix_global->process_list){
    MC_ignore_heap(&(process->process_hookup), sizeof(process->process_hookup));
  }

  if(raw_mem_set)
    MC_SET_RAW_MEM;

}

static void MC_init_dot_output(){ /* FIXME : more colors */

  colors[0] = "blue";
  colors[1] = "red";
  colors[2] = "green3";
  colors[3] = "goldenrod";
  colors[4] = "brown";
  colors[5] = "purple";
  colors[6] = "magenta";
  colors[7] = "turquoise4";
  colors[8] = "gray25";
  colors[9] = "forestgreen";
  colors[10] = "hotpink";
  colors[11] = "lightblue";
  colors[12] = "tan";

  dot_output = fopen(_sg_mc_dot_output_file, "w");
  
  if(dot_output == NULL){
    perror("Error open dot output file");
    xbt_abort();
  }

  fprintf(dot_output, "digraph graphname{\n fixedsize=true; rankdir=TB; ranksep=.25; edge [fontsize=12]; node [fontsize=10, shape=circle,width=.5 ]; graph [resolution=20, fontsize=10];\n");

}

/*******************************  Core of MC *******************************/
/**************************************************************************/

void MC_do_the_modelcheck_for_real() {

  MC_SET_RAW_MEM;
  mc_comp_times = xbt_new0(s_mc_comparison_times_t, 1);
  MC_UNSET_RAW_MEM;
  
  if (!_sg_mc_property_file || _sg_mc_property_file[0]=='\0') {
    if (mc_reduce_kind==e_mc_reduce_unset)
      mc_reduce_kind=e_mc_reduce_dpor;

    XBT_INFO("Check a safety property");
    MC_modelcheck_safety();

  } else  {

    if (mc_reduce_kind==e_mc_reduce_unset)
      mc_reduce_kind=e_mc_reduce_none;

    XBT_INFO("Check the liveness property %s",_sg_mc_property_file);
    MC_automaton_load(_sg_mc_property_file);
    MC_modelcheck_liveness();
  }
}

void MC_modelcheck_safety(void)
{
  int raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  /* Check if MC is already initialized */
  if (initial_state_safety)
    return;

  mc_time = xbt_new0(double, simix_process_maxpid);

  /* mc_time refers to clock for each process -> ignore it for heap comparison */  
  MC_ignore_heap(mc_time, simix_process_maxpid * sizeof(double));

  /* Initialize the data structures that must be persistent across every
     iteration of the model-checker (in RAW memory) */
  
  MC_SET_RAW_MEM;

  /* Initialize statistics */
  mc_stats = xbt_new0(s_mc_stats_t, 1);
  mc_stats->state_size = 1;

  /* Create exploration stack */
  mc_stack_safety = xbt_fifo_new();

  if((_sg_mc_dot_output_file != NULL) && (_sg_mc_dot_output_file[0]!='\0'))
    MC_init_dot_output();

  MC_UNSET_RAW_MEM;

  if(_sg_mc_visited > 0){
    MC_init();
  }else{
    MC_SET_RAW_MEM;
    MC_init_memory_map_info();
    MC_get_libsimgrid_plt_section();
    MC_get_binary_plt_section();
    MC_UNSET_RAW_MEM;
  }

  MC_dpor_init();

  MC_SET_RAW_MEM;
  /* Save the initial state */
  initial_state_safety = xbt_new0(s_mc_global_t, 1);
  initial_state_safety->snapshot = MC_take_snapshot();
  MC_UNSET_RAW_MEM;

  MC_dpor();

  if(raw_mem_set)
    MC_SET_RAW_MEM;

  xbt_abort();
  //MC_exit();
}

void MC_modelcheck_liveness(){

  int raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  MC_init();

  mc_time = xbt_new0(double, simix_process_maxpid);

  /* mc_time refers to clock for each process -> ignore it for heap comparison */  
  MC_ignore_heap(mc_time, simix_process_maxpid * sizeof(double));
 
  MC_SET_RAW_MEM;
 
  /* Initialize statistics */
  mc_stats = xbt_new0(s_mc_stats_t, 1);
  mc_stats->state_size = 1;

  /* Create exploration stack */
  mc_stack_liveness = xbt_fifo_new();

  /* Create the initial state */
  initial_state_liveness = xbt_new0(s_mc_global_t, 1);

  if((_sg_mc_dot_output_file != NULL) && (_sg_mc_dot_output_file[0]!='\0'))
    MC_init_dot_output();
  
  MC_UNSET_RAW_MEM;

  MC_ddfs_init();

  /* We're done */
  MC_print_statistics(mc_stats);
  xbt_free(mc_time);

  if(raw_mem_set)
    MC_SET_RAW_MEM;

}


void MC_exit(void)
{
  xbt_free(mc_time);
  MC_memory_exit();
  //xbt_abort();
}

int SIMIX_pre_mc_random(smx_simcall_t simcall, int min, int max){

  return simcall->mc_value;
}


int MC_random(int min, int max)
{
  /*FIXME: return mc_current_state->executed_transition->random.value;*/
  return simcall_mc_random(min, max);
}

/**
 * \brief Schedules all the process that are ready to run
 */
void MC_wait_for_requests(void)
{
  smx_process_t process;
  smx_simcall_t req;
  unsigned int iter;

  while (!xbt_dynar_is_empty(simix_global->process_to_run)) {
    SIMIX_process_runall();
    xbt_dynar_foreach(simix_global->process_that_ran, iter, process) {
      req = &process->simcall;
      if (req->call != SIMCALL_NONE && !MC_request_is_visible(req))
        SIMIX_simcall_pre(req, 0);
    }
  }
}

int MC_deadlock_check()
{
  int deadlock = FALSE;
  smx_process_t process;
  if(xbt_swag_size(simix_global->process_list)){
    deadlock = TRUE;
    xbt_swag_foreach(process, simix_global->process_list){
      if(process->simcall.call != SIMCALL_NONE
         && MC_request_is_enabled(&process->simcall)){
        deadlock = FALSE;
        break;
      }
    }
  }
  return deadlock;
}

/**
 * \brief Re-executes from the state at position start all the transitions indicated by
 *        a given model-checker stack.
 * \param stack The stack with the transitions to execute.
 * \param start Start index to begin the re-execution.
 */
void MC_replay(xbt_fifo_t stack, int start)
{
  int raw_mem = (mmalloc_get_current_heap() == raw_heap);

  int value, i = 1, count = 1;
  char *req_str;
  smx_simcall_t req = NULL, saved_req = NULL;
  xbt_fifo_item_t item, start_item;
  mc_state_t state;
  smx_process_t process = NULL;

  XBT_DEBUG("**** Begin Replay ****");

  if(start == -1){
    /* Restore the initial state */
    MC_restore_snapshot(initial_state_safety->snapshot);
    /* At the moment of taking the snapshot the raw heap was set, so restoring
     * it will set it back again, we have to unset it to continue  */
    MC_UNSET_RAW_MEM;
  }

  start_item = xbt_fifo_get_last_item(stack);
  if(start != -1){
    while (i != start){
      start_item = xbt_fifo_get_prev_item(start_item);
      i++;
    }
  }

  MC_SET_RAW_MEM;
  xbt_dict_reset(first_enabled_state);
  xbt_swag_foreach(process, simix_global->process_list){
    if(MC_process_is_enabled(process)){
      char *key = bprintf("%lu", process->pid);
      char *data = bprintf("%d", count);
      xbt_dict_set(first_enabled_state, key, data, NULL);
      xbt_free(key);
    }
  }
  MC_UNSET_RAW_MEM;
  

  /* Traverse the stack from the state at position start and re-execute the transitions */
  for (item = start_item;
       item != xbt_fifo_get_first_item(stack);
       item = xbt_fifo_get_prev_item(item)) {

    state = (mc_state_t) xbt_fifo_get_item_content(item);
    saved_req = MC_state_get_executed_request(state, &value);
   
    MC_SET_RAW_MEM;
    char *key = bprintf("%lu", saved_req->issuer->pid);
    xbt_dict_remove(first_enabled_state, key); 
    xbt_free(key);
    MC_UNSET_RAW_MEM;
   
    if(saved_req){
      /* because we got a copy of the executed request, we have to fetch the  
         real one, pointed by the request field of the issuer process */
      req = &saved_req->issuer->simcall;

      /* Debug information */
      if(XBT_LOG_ISENABLED(mc_global, xbt_log_priority_debug)){
        req_str = MC_request_to_string(req, value);
        XBT_DEBUG("Replay: %s (%p)", req_str, state);
        xbt_free(req_str);
      }
    }
    
    SIMIX_simcall_pre(req, value);
    MC_wait_for_requests();

    count++;

    MC_SET_RAW_MEM;
    /* Insert in dict all enabled processes */
    xbt_swag_foreach(process, simix_global->process_list){
      if(MC_process_is_enabled(process) /*&& !MC_state_process_is_done(state, process)*/){
        char *key = bprintf("%lu", process->pid);
        if(xbt_dict_get_or_null(first_enabled_state, key) == NULL){
          char *data = bprintf("%d", count);
          xbt_dict_set(first_enabled_state, key, data, NULL);
        }
        xbt_free(key);
      }
    }
    MC_UNSET_RAW_MEM;
         
    /* Update statistics */
    mc_stats->visited_states++;
    mc_stats->executed_transitions++;

  }

  XBT_DEBUG("**** End Replay ****");

  if(raw_mem)
    MC_SET_RAW_MEM;
  else
    MC_UNSET_RAW_MEM;
  

}

void MC_replay_liveness(xbt_fifo_t stack, int all_stack)
{

  initial_state_liveness->raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  int value;
  char *req_str;
  smx_simcall_t req = NULL, saved_req = NULL;
  xbt_fifo_item_t item;
  mc_state_t state;
  mc_pair_t pair;
  int depth = 1;

  XBT_DEBUG("**** Begin Replay ****");

  /* Restore the initial state */
  MC_restore_snapshot(initial_state_liveness->snapshot);

  /* At the moment of taking the snapshot the raw heap was set, so restoring
   * it will set it back again, we have to unset it to continue  */
  if(!initial_state_liveness->raw_mem_set)
    MC_UNSET_RAW_MEM;

  if(all_stack){

    item = xbt_fifo_get_last_item(stack);

    while(depth <= xbt_fifo_size(stack)){

      pair = (mc_pair_t) xbt_fifo_get_item_content(item);
      state = (mc_state_t) pair->graph_state;

      if(pair->requests > 0){
   
        saved_req = MC_state_get_executed_request(state, &value);
        //XBT_DEBUG("SavedReq->call %u", saved_req->call);
      
        if(saved_req != NULL){
          /* because we got a copy of the executed request, we have to fetch the  
             real one, pointed by the request field of the issuer process */
          req = &saved_req->issuer->simcall;
          //XBT_DEBUG("Req->call %u", req->call);
  
          /* Debug information */
          if(XBT_LOG_ISENABLED(mc_global, xbt_log_priority_debug)){
            req_str = MC_request_to_string(req, value);
            XBT_DEBUG("Replay (depth = %d) : %s (%p)", depth, req_str, state);
            xbt_free(req_str);
          }
  
        }
 
        SIMIX_simcall_pre(req, value);
        MC_wait_for_requests();
      }

      depth++;
    
      /* Update statistics */
      mc_stats->visited_pairs++;
      mc_stats->executed_transitions++;

      item = xbt_fifo_get_prev_item(item);
    }

  }else{

    /* Traverse the stack from the initial state and re-execute the transitions */
    for (item = xbt_fifo_get_last_item(stack);
         item != xbt_fifo_get_first_item(stack);
         item = xbt_fifo_get_prev_item(item)) {

      pair = (mc_pair_t) xbt_fifo_get_item_content(item);
      state = (mc_state_t) pair->graph_state;

      if(pair->requests > 0){
   
        saved_req = MC_state_get_executed_request(state, &value);
        //XBT_DEBUG("SavedReq->call %u", saved_req->call);
      
        if(saved_req != NULL){
          /* because we got a copy of the executed request, we have to fetch the  
             real one, pointed by the request field of the issuer process */
          req = &saved_req->issuer->simcall;
          //XBT_DEBUG("Req->call %u", req->call);
  
          /* Debug information */
          if(XBT_LOG_ISENABLED(mc_global, xbt_log_priority_debug)){
            req_str = MC_request_to_string(req, value);
            XBT_DEBUG("Replay (depth = %d) : %s (%p)", depth, req_str, state);
            xbt_free(req_str);
          }
  
        }
 
        SIMIX_simcall_pre(req, value);
        MC_wait_for_requests();
      }

      depth++;
    
      /* Update statistics */
      mc_stats->visited_pairs++;
      mc_stats->executed_transitions++;
    }
  }  

  XBT_DEBUG("**** End Replay ****");

  if(initial_state_liveness->raw_mem_set)
    MC_SET_RAW_MEM;
  else
    MC_UNSET_RAW_MEM;
  
}

/**
 * \brief Dumps the contents of a model-checker's stack and shows the actual
 *        execution trace
 * \param stack The stack to dump
 */
void MC_dump_stack_safety(xbt_fifo_t stack)
{
  
  int raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  MC_show_stack_safety(stack);

  if(!_sg_mc_checkpoint){

    mc_state_t state;

    MC_SET_RAW_MEM;
    while ((state = (mc_state_t) xbt_fifo_pop(stack)) != NULL)
      MC_state_delete(state);
    MC_UNSET_RAW_MEM;

  }

  if(raw_mem_set)
    MC_SET_RAW_MEM;
  else
    MC_UNSET_RAW_MEM;
  
}


void MC_show_stack_safety(xbt_fifo_t stack)
{

  int raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  MC_SET_RAW_MEM;

  int value;
  mc_state_t state;
  xbt_fifo_item_t item;
  smx_simcall_t req;
  char *req_str = NULL;
  
  for (item = xbt_fifo_get_last_item(stack);
       (item ? (state = (mc_state_t) (xbt_fifo_get_item_content(item)))
        : (NULL)); item = xbt_fifo_get_prev_item(item)) {
    req = MC_state_get_executed_request(state, &value);
    if(req){
      req_str = MC_request_to_string(req, value);
      XBT_INFO("%s", req_str);
      xbt_free(req_str);
    }
  }

  if(!raw_mem_set)
    MC_UNSET_RAW_MEM;
}

void MC_show_deadlock(smx_simcall_t req)
{
  /*char *req_str = NULL;*/
  XBT_INFO("**************************");
  XBT_INFO("*** DEAD-LOCK DETECTED ***");
  XBT_INFO("**************************");
  XBT_INFO("Locked request:");
  /*req_str = MC_request_to_string(req);
    XBT_INFO("%s", req_str);
    xbt_free(req_str);*/
  XBT_INFO("Counter-example execution trace:");
  MC_dump_stack_safety(mc_stack_safety);
  MC_print_statistics(mc_stats);
}


void MC_show_stack_liveness(xbt_fifo_t stack){
  int value;
  mc_pair_t pair;
  xbt_fifo_item_t item;
  smx_simcall_t req;
  char *req_str = NULL;
  
  for (item = xbt_fifo_get_last_item(stack);
       (item ? (pair = (mc_pair_t) (xbt_fifo_get_item_content(item)))
        : (NULL)); item = xbt_fifo_get_prev_item(item)) {
    req = MC_state_get_executed_request(pair->graph_state, &value);
    if(req){
      if(pair->requests>0){
        req_str = MC_request_to_string(req, value);
        XBT_INFO("%s", req_str);
        xbt_free(req_str);
      }else{
        XBT_INFO("End of system requests but evolution in Büchi automaton");
      }
    }
  }
}

void MC_dump_stack_liveness(xbt_fifo_t stack){

  int raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  mc_pair_t pair;

  MC_SET_RAW_MEM;
  while ((pair = (mc_pair_t) xbt_fifo_pop(stack)) != NULL)
    MC_pair_delete(pair);
  MC_UNSET_RAW_MEM;

  if(raw_mem_set)
    MC_SET_RAW_MEM;

}


void MC_print_statistics(mc_stats_t stats)
{
  if(stats->expanded_pairs == 0){
    XBT_INFO("Expanded states = %lu", stats->expanded_states);
    XBT_INFO("Visited states = %lu", stats->visited_states);
  }else{
    XBT_INFO("Expanded pairs = %lu", stats->expanded_pairs);
    XBT_INFO("Visited pairs = %lu", stats->visited_pairs);
  }
  XBT_INFO("Executed transitions = %lu", stats->executed_transitions);
  MC_SET_RAW_MEM;
  if((_sg_mc_dot_output_file != NULL) && (_sg_mc_dot_output_file[0]!='\0')){
    fprintf(dot_output, "}\n");
    fclose(dot_output);
  }
  MC_UNSET_RAW_MEM;
}

void MC_assert(int prop)
{
  if (MC_is_active() && !prop){
    XBT_INFO("**************************");
    XBT_INFO("*** PROPERTY NOT VALID ***");
    XBT_INFO("**************************");
    XBT_INFO("Counter-example execution trace:");
    MC_dump_stack_safety(mc_stack_safety);
    MC_print_statistics(mc_stats);
    xbt_abort();
  }
}

void MC_cut(void){
  user_max_depth_reached = 1;
}

void MC_process_clock_add(smx_process_t process, double amount)
{
  mc_time[process->pid] += amount;
}

double MC_process_clock_get(smx_process_t process)
{
  if(mc_time){
    if(process != NULL)
      return mc_time[process->pid];
    else 
      return -1;
  }else{
    return 0;
  }
}

void MC_automaton_load(const char *file){

  int raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  MC_SET_RAW_MEM;

  if (_mc_property_automaton == NULL)
    _mc_property_automaton = xbt_automaton_new();
  
  xbt_automaton_load(_mc_property_automaton,file);

  MC_UNSET_RAW_MEM;

  if(raw_mem_set)
    MC_SET_RAW_MEM;

}

void MC_automaton_new_propositional_symbol(const char* id, void* fct) {

  int raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  MC_SET_RAW_MEM;

  if (_mc_property_automaton == NULL)
    _mc_property_automaton = xbt_automaton_new();

  xbt_automaton_propositional_symbol_new(_mc_property_automaton,id,fct);

  MC_UNSET_RAW_MEM;

  if(raw_mem_set)
    MC_SET_RAW_MEM;
  
}



