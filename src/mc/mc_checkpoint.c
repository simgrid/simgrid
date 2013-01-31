/* Copyright (c) 2008-2012 Da SimGrid Team. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <libgen.h>
#include "mc_private.h"
#include "xbt/module.h"

#include "../simix/smx_private.h"

#include <libunwind.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_checkpoint, mc,
                                "Logging specific to mc_checkpoint");

void *start_text_libsimgrid;
void *start_plt_libsimgrid, *end_plt_libsimgrid;
void *start_got_plt_libsimgrid, *end_got_plt_libsimgrid;
void *start_plt_binary, *end_plt_binary;
void *start_got_plt_binary, *end_got_plt_binary;
char *libsimgrid_path;
void *start_data_libsimgrid, *start_bss_libsimgrid;
void *start_data_binary, *start_bss_binary;
void *start_text_binary;

static mc_mem_region_t MC_region_new(int type, void *start_addr, size_t size);
static void MC_region_restore(mc_mem_region_t reg);
static void MC_region_destroy(mc_mem_region_t reg);

static void MC_snapshot_add_region(mc_snapshot_t snapshot, int type, void *start_addr, size_t size);

static void add_value(xbt_dynar_t *list, const char *type, unsigned long int val);
static xbt_dynar_t take_snapshot_stacks(mc_snapshot_t *s, void *heap);
static xbt_strbuff_t get_local_variables_values(void *stack_context, void *heap);
static void print_local_variables_values(xbt_dynar_t all_variables);
static void *get_stack_pointer(void *stack_context, void *heap);

static void snapshot_stack_free(mc_snapshot_stack_t s);
static xbt_dynar_t take_snapshot_ignore(void);

static mc_mem_region_t MC_region_new(int type, void *start_addr, size_t size)
{
  mc_mem_region_t new_reg = xbt_new0(s_mc_mem_region_t, 1);
  new_reg->type = type;
  new_reg->start_addr = start_addr;
  new_reg->size = size;
  new_reg->data = xbt_malloc0(size);
  memcpy(new_reg->data, start_addr, size);

  XBT_DEBUG("New region : type : %d, data : %p (real addr %p), size : %zu", type, new_reg->data, start_addr, size);
  
  return new_reg;
}

static void MC_region_restore(mc_mem_region_t reg)
{
  /*FIXME: check if start_addr is still mapped, if it is not, then map it
    before copying the data */
 
  memcpy(reg->start_addr, reg->data, reg->size);
 
  return;
}

static void MC_region_destroy(mc_mem_region_t reg)
{
  xbt_free(reg->data);
  xbt_free(reg);
}

static void MC_snapshot_add_region(mc_snapshot_t snapshot, int type, void *start_addr, size_t size)
{
  mc_mem_region_t new_reg = MC_region_new(type, start_addr, size);
  snapshot->regions = xbt_realloc(snapshot->regions, (snapshot->num_reg + 1) * sizeof(mc_mem_region_t));
  snapshot->regions[snapshot->num_reg] = new_reg;
  snapshot->region_type[snapshot->num_reg] = type;
  snapshot->num_reg++;
  return;
} 

void MC_init_memory_map_info(){

  int raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  MC_SET_RAW_MEM;
  
  unsigned int i = 0;
  s_map_region_t reg;
  memory_map_t maps = get_memory_map();

  while (i < maps->mapsize) {
    reg = maps->regions[i];
    if ((reg.prot & PROT_WRITE)){
      if (maps->regions[i].pathname != NULL){
        if (!memcmp(basename(maps->regions[i].pathname), "libsimgrid", 10)){
          start_data_libsimgrid = reg.start_addr;
          i++;
          reg = maps->regions[i];
          if(reg.pathname == NULL && (reg.prot & PROT_WRITE) && i < maps->mapsize)
            start_bss_libsimgrid = reg.start_addr;
        }else if (!memcmp(basename(maps->regions[i].pathname), basename(xbt_binary_name), strlen(basename(xbt_binary_name)))){
          start_data_binary = reg.start_addr;
          i++;
          reg = maps->regions[i];
          if(reg.pathname == NULL && (reg.prot & PROT_WRITE) && reg.start_addr != std_heap && reg.start_addr != raw_heap && i < maps->mapsize){
            start_bss_binary = reg.start_addr;
            i++;
          }
        }else if(!memcmp(maps->regions[i].pathname, "[stack]", 7)){
          maestro_stack_start = reg.start_addr;
          maestro_stack_end = reg.end_addr;
          i++;
        }
      }
    }else if ((reg.prot & PROT_READ) && (reg.prot & PROT_EXEC)){
      if (maps->regions[i].pathname != NULL){
        if (!memcmp(basename(maps->regions[i].pathname), "libsimgrid", 10)){
          start_text_libsimgrid = reg.start_addr;
          libsimgrid_path = strdup(maps->regions[i].pathname);
        }else if (!memcmp(basename(maps->regions[i].pathname), basename(xbt_binary_name), strlen(basename(xbt_binary_name)))){
          start_text_binary = reg.start_addr;
        }
      }
    }
    i++;
  }
  
  free_memory_map(maps);

  MC_UNSET_RAW_MEM;

  if(raw_mem_set)
    MC_SET_RAW_MEM;

}

mc_snapshot_t SIMIX_pre_mc_snapshot(smx_simcall_t simcall){
  return MC_take_snapshot();
}

mc_snapshot_t MC_take_snapshot()
{
  int raw_mem = (mmalloc_get_current_heap() == raw_heap);
  
  MC_SET_RAW_MEM;

  mc_snapshot_t snapshot = xbt_new0(s_mc_snapshot_t, 1);
  snapshot->nb_processes = xbt_swag_size(simix_global->process_list);

  unsigned int i = 0;
  s_map_region_t reg;
  memory_map_t maps = get_memory_map();
  void *heap = NULL;
  size_t size = 0;
  void *start = NULL;

  /* Save the std heap and the writable mapped pages of libsimgrid */
  while (i < maps->mapsize) {
    reg = maps->regions[i];
    if ((reg.prot & PROT_WRITE)){
      if (maps->regions[i].pathname == NULL){
        if (reg.start_addr == std_heap){ // only save the std heap (and not the raw one)
          MC_snapshot_add_region(snapshot, 0, reg.start_addr, (char*)reg.end_addr - (char*)reg.start_addr);
          snapshot->heap_bytes_used = mmalloc_get_bytes_used(std_heap);
          heap = snapshot->regions[snapshot->num_reg - 1]->data;
        }
        i++;
      } else{ 
        if (!memcmp(basename(maps->regions[i].pathname), "libsimgrid", 10)){
          size = (char*)reg.end_addr - (char*)reg.start_addr;
          start = reg.start_addr;
          i++;
          reg = maps->regions[i];
          if(reg.pathname == NULL && (reg.prot & PROT_WRITE) && i < maps->mapsize){
            size += (char*)reg.end_addr - (char*)reg.start_addr;
            reg = maps->regions[i];
            i++;
          }
          MC_snapshot_add_region(snapshot, 1, start, size);
        }else if(!memcmp(maps->regions[i].pathname, "[stack]", 7)){
          maestro_stack_start = reg.start_addr;
          maestro_stack_end = reg.end_addr;
          i++;
        } else if (!memcmp(basename(maps->regions[i].pathname), basename(xbt_binary_name), strlen(basename(xbt_binary_name)))){
          size = (char*)reg.end_addr - (char*)reg.start_addr;
          start = reg.start_addr;
          i++;
          reg = maps->regions[i];
          if(reg.pathname == NULL && (reg.prot & PROT_WRITE) && reg.start_addr != std_heap && reg.start_addr != raw_heap && i < maps->mapsize){
            size += (char*)reg.end_addr - (char*)reg.start_addr;
            reg = maps->regions[i];
            i++;
          }
          MC_snapshot_add_region(snapshot, 2, start, size);
        }else{
          i++;
        }
      }
    }else{
      i++;
    }
  }

  snapshot->to_ignore = take_snapshot_ignore();

  if(_sg_mc_visited > 0 || strcmp(_sg_mc_property_file,""))
    snapshot->stacks = take_snapshot_stacks(&snapshot, heap);
  
  free_memory_map(maps);

  MC_UNSET_RAW_MEM;

  if(raw_mem)
    MC_SET_RAW_MEM;

  return snapshot;

}

void MC_restore_snapshot(mc_snapshot_t snapshot)
{
  unsigned int i;
  for(i=0; i < snapshot->num_reg; i++){
    MC_region_restore(snapshot->regions[i]);
  }

}

void MC_free_snapshot(mc_snapshot_t snapshot)
{
  unsigned int i;
  for(i=0; i < snapshot->num_reg; i++)
    MC_region_destroy(snapshot->regions[i]);

  xbt_free(snapshot->regions);
  xbt_dynar_free(&(snapshot->stacks));
  xbt_dynar_free(&(snapshot->to_ignore));
  xbt_free(snapshot);
}


void get_libsimgrid_plt_section(){

  FILE *fp;
  char *line = NULL;            /* Temporal storage for each line that is readed */
  ssize_t read;                 /* Number of bytes readed */
  size_t n = 0;                 /* Amount of bytes to read by getline */

  char *lfields[7];
  int i, plt_found = 0;
  unsigned long int size, offset;

  char *command = bprintf("objdump --section-headers %s", libsimgrid_path);

  fp = popen(command, "r");

  if(fp == NULL){
    perror("popen failed");
    xbt_abort();
  }

  while ((read = getline(&line, &n, fp)) != -1 && plt_found != 2) {

    if(n == 0)
      continue;

    /* Wipeout the new line character */
    line[read - 1] = '\0';

    lfields[0] = strtok(line, " ");

    if(lfields[0] == NULL)
      continue;

    if(strcmp(lfields[0], "Sections:") == 0 || strcmp(lfields[0], "Idx") == 0 || strncmp(lfields[0], libsimgrid_path, strlen(libsimgrid_path)) == 0)
      continue;

    for (i = 1; i < 7 && lfields[i - 1] != NULL; i++) {
      lfields[i] = strtok(NULL, " ");
    }

    if(i>=6){
      if(strcmp(lfields[1], ".plt") == 0){
        size = strtoul(lfields[2], NULL, 16);
        offset = strtoul(lfields[5], NULL, 16);
        start_plt_libsimgrid = (char *)start_text_libsimgrid + offset;
        end_plt_libsimgrid = (char *)start_plt_libsimgrid + size;
        plt_found++;
      }else if(strcmp(lfields[1], ".got.plt") == 0){
        size = strtoul(lfields[2], NULL, 16);
        offset = strtoul(lfields[5], NULL, 16);
        start_got_plt_libsimgrid = (char *)start_text_libsimgrid + offset;
        end_got_plt_libsimgrid = (char *)start_got_plt_libsimgrid + size;
        plt_found++;
       }

    }
    
  }

  xbt_free(command);
  xbt_free(line);
  pclose(fp);

}

void get_binary_plt_section(){

  FILE *fp;
  char *line = NULL;            /* Temporal storage for each line that is readed */
  ssize_t read;                 /* Number of bytes readed */
  size_t n = 0;                 /* Amount of bytes to read by getline */

  char *lfields[7];
  int i, plt_found = 0;
  unsigned long int size;

  char *command = bprintf( "objdump --section-headers %s", xbt_binary_name);

  fp = popen(command, "r");

  if(fp == NULL){
    perror("popen failed");
    xbt_abort();
  }

  while ((read = getline(&line, &n, fp)) != -1 && plt_found != 2) {

    if(n == 0)
      continue;

    /* Wipeout the new line character */
    line[read - 1] = '\0';

    lfields[0] = strtok(line, " ");

    if(lfields[0] == NULL)
      continue;

    if(strcmp(lfields[0], "Sections:") == 0 || strcmp(lfields[0], "Idx") == 0 || strncmp(lfields[0], basename(xbt_binary_name), strlen(xbt_binary_name)) == 0)
      continue;

    for (i = 1; i < 7 && lfields[i - 1] != NULL; i++) {
      lfields[i] = strtok(NULL, " ");
    }

    if(i>=6){
      if(strcmp(lfields[1], ".plt") == 0){
        size = strtoul(lfields[2], NULL, 16);
        start_plt_binary = (void *)strtoul(lfields[3], NULL, 16);
        end_plt_binary = (char *)start_plt_binary + size;
        plt_found++;
      }else if(strcmp(lfields[1], ".got.plt") == 0){
        size = strtoul(lfields[2], NULL, 16);
        start_got_plt_binary = (char *)strtoul(lfields[3], NULL, 16);
        end_got_plt_binary = (char *)start_got_plt_binary + size;
        plt_found++;
       }
    }
    
    
  }

  xbt_free(command);
  xbt_free(line);
  pclose(fp);

}

static void add_value(xbt_dynar_t *list, const char *type, unsigned long int val){
  variable_value_t value = xbt_new0(s_variable_value_t, 1);
  value->type = strdup(type);
  if(strcmp(type, "value") == 0){
    value->value.res = val;
  }else{
    value->value.address = (void *)val;
  }
  xbt_dynar_push(*list, &value);
}

static xbt_dynar_t take_snapshot_stacks(mc_snapshot_t *snapshot, void *heap){

  xbt_dynar_t res = xbt_dynar_new(sizeof(s_mc_snapshot_stack_t), snapshot_stack_free_voidp);

  unsigned int cursor = 0;
  stack_region_t current_stack;
  
  xbt_dynar_foreach(stacks_areas, cursor, current_stack){
    mc_snapshot_stack_t st = xbt_new(s_mc_snapshot_stack_t, 1);
    st->local_variables = get_local_variables_values(current_stack->context, heap);
    st->stack_pointer = get_stack_pointer(current_stack->context, heap);
    xbt_dynar_push(res, &st);
    (*snapshot)->stack_sizes = xbt_realloc((*snapshot)->stack_sizes, (cursor + 1) * sizeof(size_t));
    (*snapshot)->stack_sizes[cursor] = current_stack->size - ((char *)st->stack_pointer - (char *)((char *)heap + ((char *)current_stack->address - (char *)std_heap)));
  }

  return res;

}

static void *get_stack_pointer(void *stack_context, void *heap){

  unw_cursor_t c;
  int ret;
  unw_word_t sp;

  ret = unw_init_local(&c, (unw_context_t *)stack_context);
  if(ret < 0){
    XBT_INFO("unw_init_local failed");
    xbt_abort();
  }

  unw_get_reg(&c, UNW_REG_SP, &sp);

  return ((char *)heap + (size_t)(((char *)((long)sp) - (char*)std_heap)));

}

static xbt_strbuff_t get_local_variables_values(void *stack_context, void *heap){
  
  unw_cursor_t c;
  int ret;

  char frame_name[256];
  
  ret = unw_init_local(&c, (unw_context_t *)stack_context);
  if(ret < 0){
    XBT_INFO("unw_init_local failed");
    xbt_abort();
  }

  unw_word_t ip, sp, off;
  dw_frame_t frame;
 
  xbt_dynar_t compose = xbt_dynar_new(sizeof(variable_value_t), variable_value_free_voidp);

  xbt_strbuff_t variables = xbt_strbuff_new();
  xbt_dict_cursor_t dict_cursor;
  char *variable_name;
  dw_local_variable_t current_variable;
  unsigned int cursor = 0, cursor2 = 0;
  dw_location_entry_t entry = NULL;
  dw_location_t location_entry = NULL;
  unw_word_t res;
  int frame_found = 0;
  void *frame_pointer_address = NULL;
  long true_ip;
  char *to_append;

  while(ret >= 0){

    unw_get_reg(&c, UNW_REG_IP, &ip);
    unw_get_reg(&c, UNW_REG_SP, &sp);

    unw_get_proc_name(&c, frame_name, sizeof (frame_name), &off);

    frame = xbt_dict_get_or_null(mc_local_variables, frame_name);

    if(frame == NULL){
      xbt_dynar_free(&compose);
      xbt_dict_cursor_free(&dict_cursor);
      return variables;
    }

    to_append = bprintf("ip=%s\n", frame_name);
    xbt_strbuff_append(variables, to_append);
    xbt_free(to_append);

    true_ip = (long)frame->low_pc + (long)off;

    /* Get frame pointer */
    switch(frame->frame_base->type){
    case e_dw_loclist:
      while((cursor < xbt_dynar_length(frame->frame_base->location.loclist)) && frame_found == 0){
        entry = xbt_dynar_get_as(frame->frame_base->location.loclist, cursor, dw_location_entry_t);
        if((true_ip >= entry->lowpc) && (true_ip < entry->highpc)){
          frame_found = 1;
          switch(entry->location->type){
          case e_dw_compose:
            xbt_dynar_reset(compose);
            cursor2 = 0;
            while(cursor2 < xbt_dynar_length(entry->location->location.compose)){
              location_entry = xbt_dynar_get_as(entry->location->location.compose, cursor2, dw_location_t);
              switch(location_entry->type){
              case e_dw_register:
                unw_get_reg(&c, location_entry->location.reg, &res);
                add_value(&compose, "address", (long)res);
                break;
              case e_dw_bregister_op:
                unw_get_reg(&c, location_entry->location.breg_op.reg, &res);
                add_value(&compose, "address", (long)res + location_entry->location.breg_op.offset);
                break;
              default:
                xbt_dynar_reset(compose);
                break;
              }
              cursor2++;
            }

            if(xbt_dynar_length(compose) > 0){
              frame_pointer_address = xbt_dynar_get_as(compose, xbt_dynar_length(compose) - 1, variable_value_t)->value.address ; 
            }
            break;
          default :
            frame_pointer_address = NULL;
            break;
          }
        }
        cursor++;
      }
      break;
    default :
      frame_pointer_address = NULL;
      break;
    }

    frame_found = 0;
    cursor = 0;

    xbt_dict_foreach(frame->variables, dict_cursor, variable_name, current_variable){
      if(current_variable->location != NULL){
        switch(current_variable->location->type){
        case e_dw_compose:
          xbt_dynar_reset(compose);
          cursor = 0;
          while(cursor < xbt_dynar_length(current_variable->location->location.compose)){
            location_entry = xbt_dynar_get_as(current_variable->location->location.compose, cursor, dw_location_t);
            switch(location_entry->type){
            case e_dw_register:
              unw_get_reg(&c, location_entry->location.reg, &res);
              add_value(&compose, "value", (long)res);
              break;
            case e_dw_bregister_op:
              unw_get_reg(&c, location_entry->location.breg_op.reg, &res);
              add_value(&compose, "address", (long)res + location_entry->location.breg_op.offset);
              break;
            case e_dw_fbregister_op:
              if(frame_pointer_address != NULL)
                add_value(&compose, "address", (long)((char *)frame_pointer_address + location_entry->location.fbreg_op));
              break;
            default:
              xbt_dynar_reset(compose);
              break;
            }
            cursor++;
          }
          
          if(xbt_dynar_length(compose) > 0){
            if(strcmp(xbt_dynar_get_as(compose, xbt_dynar_length(compose) - 1, variable_value_t)->type, "value") == 0){
              to_append = bprintf("%s=%lx\n", current_variable->name, xbt_dynar_get_as(compose, xbt_dynar_length(compose) - 1, variable_value_t)->value.res);
              xbt_strbuff_append(variables, to_append);
              xbt_free(to_append);
            }else{
              if((long)xbt_dynar_get_as(compose, xbt_dynar_length(compose) - 1,variable_value_t)->value.address < 0 || *((void**)xbt_dynar_get_as(compose, xbt_dynar_length(compose) - 1,variable_value_t)->value.address) == NULL){
                to_append = bprintf("%s=NULL\n", current_variable->name);
                xbt_strbuff_append(variables, to_append);
                xbt_free(to_append);
              }else if(((long)*((void**)xbt_dynar_get_as(compose, xbt_dynar_length(compose) - 1,variable_value_t)->value.address) > 0xffffffff) || ((long)*((void**)xbt_dynar_get_as(compose, xbt_dynar_length(compose) - 1,variable_value_t)->value.address) < (long)start_text_binary)){
                to_append = bprintf("%s=%u\n", current_variable->name, (unsigned int)(long)*((void**)xbt_dynar_get_as(compose, xbt_dynar_length(compose) - 1, variable_value_t)->value.address));
                xbt_strbuff_append(variables, to_append);
                xbt_free(to_append);
              }else{ 
                to_append = bprintf("%s=%p\n", current_variable->name, *((void**)xbt_dynar_get_as(compose, xbt_dynar_length(compose) - 1, variable_value_t)->value.address));
                xbt_strbuff_append(variables, to_append);
                xbt_free(to_append);
              }
            }
          }else{
            to_append = bprintf("%s=undefined\n", current_variable->name);
            xbt_strbuff_append(variables, to_append);
            xbt_free(to_append);
          }
          break;
        default :
          break;
        }
      }else{
        to_append = bprintf("%s=undefined\n", current_variable->name);
        xbt_strbuff_append(variables, to_append);
        xbt_free(to_append);
      }
    }    
 
    ret = unw_step(&c);
     
  }

  xbt_dynar_free(&compose);
  xbt_dict_cursor_free(&dict_cursor);

  return variables;

}

static void print_local_variables_values(xbt_dynar_t all_variables){

  unsigned cursor = 0;
  mc_snapshot_stack_t stack;

  xbt_dynar_foreach(all_variables, cursor, stack){
    XBT_INFO("%s", stack->local_variables->data);
  }
}


static void snapshot_stack_free(mc_snapshot_stack_t s){
  if(s){
    xbt_free(s->local_variables->data);
    xbt_free(s->local_variables);
    xbt_free(s);
  }
}

void snapshot_stack_free_voidp(void *s){
  snapshot_stack_free((mc_snapshot_stack_t) * (void **) s);
}

void *MC_snapshot(void){

  return simcall_mc_snapshot();
  
}

void variable_value_free(variable_value_t v){
  if(v){
    xbt_free(v->type);
    xbt_free(v);
  }
}

void variable_value_free_voidp(void* v){
  variable_value_free((variable_value_t) * (void **)v);
}

static xbt_dynar_t take_snapshot_ignore(){
  
  if(mc_heap_comparison_ignore == NULL)
    return NULL;

  xbt_dynar_t cpy = xbt_dynar_new(sizeof(mc_heap_ignore_region_t), heap_ignore_region_free_voidp);

  unsigned int cursor = 0;
  mc_heap_ignore_region_t current_region;

  xbt_dynar_foreach(mc_heap_comparison_ignore, cursor, current_region){
    mc_heap_ignore_region_t new_region = NULL;
    new_region = xbt_new0(s_mc_heap_ignore_region_t, 1);
    new_region->address = current_region->address;
    new_region->size = current_region->size;
    new_region->block = current_region->block;
    new_region->fragment = current_region->fragment;
    xbt_dynar_push(cpy, &new_region);
  }

  return cpy;

}
