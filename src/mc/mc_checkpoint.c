/* Copyright (c) 2008-2013 Da SimGrid Team. All rights reserved.            */

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

/************************************  Free functions **************************************/
/*****************************************************************************************/

static void MC_snapshot_stack_free(mc_snapshot_stack_t s){
  if(s){
    xbt_dynar_free(&(s->local_variables));
    xbt_free(s);
  }
}

static void MC_snapshot_stack_free_voidp(void *s){
  MC_snapshot_stack_free((mc_snapshot_stack_t) * (void **) s);
}

static void local_variable_free(local_variable_t v){
  xbt_free(v->frame);
  xbt_free(v->name);
  xbt_free(v->type);
  xbt_free(v);
}

static void local_variable_free_voidp(void *v){
  local_variable_free((local_variable_t) * (void **) v);
}

static void MC_region_destroy(mc_mem_region_t reg)
{
  xbt_free(reg->data);
  xbt_free(reg);
}

void MC_free_snapshot(mc_snapshot_t snapshot){
  unsigned int i;
  for(i=0; i < NB_REGIONS; i++)
    MC_region_destroy(snapshot->regions[i]);

  xbt_free(snapshot->stack_sizes);
  xbt_dynar_free(&(snapshot->stacks));
  xbt_dynar_free(&(snapshot->to_ignore));
  xbt_free(snapshot);
}


/*******************************  Snapshot regions ********************************/
/*********************************************************************************/

static mc_mem_region_t MC_region_new(int type, void *start_addr, size_t size)
{
  mc_mem_region_t new_reg = xbt_new0(s_mc_mem_region_t, 1);
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

static void MC_snapshot_add_region(mc_snapshot_t snapshot, int type, void *start_addr, size_t size)
{
  mc_mem_region_t new_reg = MC_region_new(type, start_addr, size);
  snapshot->regions[type] = new_reg;
  return;
} 

static void MC_get_memory_regions(mc_snapshot_t snapshot){

  FILE *fp;
  char *line = NULL;
  ssize_t read;
  size_t n = 0;
  
  char *lfields[6] = {0}, *tok;
  void *start_addr, *start_addr1, *end_addr;
  size_t size;
  int i;

  fp = fopen("/proc/self/maps", "r");
  
  xbt_assert(fp, 
             "Cannot open /proc/self/maps to investigate the memory map of the process. Please report this bug.");

  setbuf(fp, NULL);

  while((read = xbt_getline(&line, &n, fp)) != -1){

    /* Wipeout the new line character */
    line[read - 1] = '\0';

    /* Tokenize the line using spaces as delimiters and store each token */
    lfields[0] = strtok(line, " ");

    for (i = 1; i < 6 && lfields[i - 1] != NULL; i++) {
      lfields[i] = strtok(NULL, " ");
    }

    /* First get the permissions flags, need write permission */
    if(lfields[1][1] == 'w'){

      /* Get the start address of the map */
      tok = strtok(lfields[0], "-");
      start_addr = (void *)strtoul(tok, NULL, 16);
    
      if(start_addr == std_heap){     /* Std_heap ? */
        tok = strtok(NULL, "-");
        end_addr = (void *)strtoul(tok, NULL, 16);
        MC_snapshot_add_region(snapshot, 0, start_addr, (char*)end_addr - (char*)start_addr);
        snapshot->heap_bytes_used = mmalloc_get_bytes_used(std_heap);
      }else{ /* map name == libsimgrid || binary_name ? */
        if(lfields[5] != NULL){
          if(!memcmp(basename(lfields[5]), "libsimgrid", 10)){
            tok = strtok(NULL, "-");
            end_addr = (void *)strtoul(tok, NULL, 16);
            size = (char*)end_addr - (char*)start_addr;
            /* BSS and data segments may be separated according to the OS */
            if((read = xbt_getline(&line, &n, fp)) != -1){
              line[read - 1] = '\0';
              lfields[0] = strtok(line, " ");
              for (i = 1; i < 6 && lfields[i - 1] != NULL; i++) {
                lfields[i] = strtok(NULL, " ");
              }
              if(lfields[1][1] == 'w' && lfields[5] == NULL){
                tok = strtok(lfields[0], "-");
                start_addr1 = (void *)strtoul(tok, NULL, 16);
                tok = strtok(NULL, "-");
                size += (char *)(void *)strtoul(tok, NULL, 16) - (char*)start_addr1;
              }
            }
            MC_snapshot_add_region(snapshot, 1, start_addr, size);
          }else if(!memcmp(basename(lfields[5]), basename(xbt_binary_name), strlen(basename(xbt_binary_name)))){
            tok = strtok(NULL, "-");
            end_addr = (void *)strtoul(tok, NULL, 16);
            size = (char*)end_addr - (char*)start_addr;
             /* BSS and data segments may be separated according to the OS */
            if((read = xbt_getline(&line, &n, fp)) != -1){
              line[read - 1] = '\0';
              lfields[0] = strtok(line, " ");
              for (i = 1; i < 6 && lfields[i - 1] != NULL; i++) {
                lfields[i] = strtok(NULL, " ");
              }
              tok = strtok(lfields[0], "-");
              start_addr1 = (void *)strtoul(tok, NULL, 16);
              if(lfields[1][1] == 'w' && lfields[5] == NULL){
                if(start_addr1 == std_heap){     /* Std_heap ? */
                  tok = strtok(NULL, "-");
                  end_addr = (void *)strtoul(tok, NULL, 16);
                  MC_snapshot_add_region(snapshot, 0, start_addr1, (char*)end_addr - (char*)start_addr1);
                  snapshot->heap_bytes_used = mmalloc_get_bytes_used(std_heap);
                }else if(start_addr1 != raw_heap){
                  tok = strtok(NULL, "-");
                  size += (char *)(void *)strtoul(tok, NULL, 16) - (char *)start_addr1;
                }
              }
            }
            MC_snapshot_add_region(snapshot, 2, start_addr, size);
          }else if (!memcmp(lfields[5], "[stack]", 7)){
            maestro_stack_start = start_addr;
            tok = strtok(NULL, "-");
            maestro_stack_end = (void *)strtoul(tok, NULL, 16);
          }
        }
      }
    }
    
  }

  free(line);
  fclose(fp);

}

void MC_init_memory_map_info(){
 
  unsigned int i = 0;
  s_map_region_t reg;
  memory_map_t maps = MC_get_memory_map();

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
   
  MC_free_memory_map(maps);

}

void MC_get_libsimgrid_plt_section(){

  FILE *fp;
  char *line = NULL;            /* Temporal storage for each line that is readed */
  ssize_t read;                 /* Number of bytes readed */
  size_t n = 0;                 /* Amount of bytes to read by xbt_getline */

  char *lfields[7];
  int i, plt_found = 0;
  unsigned long int size, offset;

  char *command = bprintf("objdump --section-headers %s", libsimgrid_path);

  fp = popen(command, "r");

  if(fp == NULL){
    perror("popen failed");
    xbt_abort();
  }

  while ((read = xbt_getline(&line, &n, fp)) != -1 && plt_found != 2) {

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

void MC_get_binary_plt_section(){

  FILE *fp;
  char *line = NULL;            /* Temporal storage for each line that is readed */
  ssize_t read;                 /* Number of bytes readed */
  size_t n = 0;                 /* Amount of bytes to read by xbt_getline */

  char *lfields[7];
  int i, plt_found = 0;
  unsigned long int size;

  char *command = bprintf( "objdump --section-headers %s", xbt_binary_name);

  fp = popen(command, "r");

  if(fp == NULL){
    perror("popen failed");
    xbt_abort();
  }

  while ((read = xbt_getline(&line, &n, fp)) != -1 && plt_found != 2) {

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

/************************************* Take Snapshot ************************************/
/****************************************************************************************/

static void MC_get_hash_global(char *snapshot_hash, void *data1, void *data2){
  
  /* unsigned int cursor = 0; */
  /* size_t offset;  */
  /* global_variable_t current_var;  */
  /* void *addr_pointed = NULL; */
  /* void *res = NULL; */

  /* xbt_strbuff_t clear = xbt_strbuff_new(); */
  
  /* xbt_dynar_foreach(mc_global_variables, cursor, current_var){ */
  /*   if(current_var->address < start_data_libsimgrid){ /\* binary *\/ */
  /*     offset = (char *)current_var->address - (char *)start_data_binary; */
  /*     addr_pointed = *((void **)((char *)data2 + offset)); */
  /*     if(((addr_pointed >= start_plt_binary && addr_pointed <= end_plt_binary)) || ((addr_pointed >= std_heap && (char *)addr_pointed <= (char *)std_heap + STD_HEAP_SIZE ))) */
  /*       continue; */
  /*     res = xbt_malloc0(current_var->size + 1); */
  /*     memset(res, 0, current_var->size + 1); */
  /*     memcpy(res, (char*)data2 + offset, current_var->size); */
  /*   }else{ /\* libsimgrid *\/ */
  /*     offset = (char *)current_var->address - (char *)start_data_libsimgrid; */
  /*     addr_pointed = *((void **)((char *)data1 + offset)); */
  /*     if((addr_pointed >= start_plt_libsimgrid && addr_pointed <= end_plt_libsimgrid) || (addr_pointed >= std_heap && (char *)addr_pointed <= (char *)std_heap + STD_HEAP_SIZE )) */
  /*       continue; */
  /*     res = xbt_malloc0(current_var->size + 1); */
  /*     memset(res, 0, current_var->size + 1); */
  /*     memcpy(res, (char*)data1 + offset, current_var->size); */
  /*   } */
  /*   if(res != NULL){ */
  /*     xbt_strbuff_append(clear, (const char*)res); */
  /*     xbt_free(res); */
  /*     res = NULL; */
  /*   } */
  /* } */

  /* xbt_sha(clear->data, snapshot_hash); */

  /* xbt_strbuff_free(clear); */

}

static void MC_get_hash_local(char *snapshot_hash, xbt_dynar_t stacks){

  /* xbt_dynar_t tokens = NULL, s_tokens = NULL; */
  /* unsigned int cursor1 = 0, cursor2 = 0; */
  /* mc_snapshot_stack_t current_stack; */
  /* char *frame_name = NULL; */
  /* void *addr; */

  /* xbt_strbuff_t clear = xbt_strbuff_new(); */

  /* while(cursor1 < xbt_dynar_length(stacks)){ */
  /*   current_stack = xbt_dynar_get_as(stacks, cursor1, mc_snapshot_stack_t); */
  /*   tokens = xbt_str_split(current_stack->local_variables->data, NULL); */
  /*   cursor2 = 0; */
  /*   while(cursor2 < xbt_dynar_length(tokens)){ */
  /*     s_tokens = xbt_str_split(xbt_dynar_get_as(tokens, cursor2, char *), "="); */
  /*     if(xbt_dynar_length(s_tokens) > 1){ */
  /*       if(strcmp(xbt_dynar_get_as(s_tokens, 0, char *), "frame_name") == 0){ */
  /*         xbt_free(frame_name); */
  /*         frame_name = xbt_strdup(xbt_dynar_get_as(s_tokens, 1, char *)); */
  /*         xbt_strbuff_append(clear, (const char*)xbt_dynar_get_as(tokens, cursor2, char *)); */
  /*         cursor2++; */
  /*         xbt_dynar_free(&s_tokens); */
  /*         continue; */
  /*       } */
  /*       addr = (void *) strtoul(xbt_dynar_get_as(s_tokens, 1, char *), NULL, 16); */
  /*       if(addr > std_heap && (char *)addr <= (char *)std_heap + STD_HEAP_SIZE){ */
  /*         cursor2++; */
  /*         xbt_dynar_free(&s_tokens); */
  /*         continue; */
  /*       } */
  /*       if(is_stack_ignore_variable(frame_name, xbt_dynar_get_as(s_tokens, 0, char *))){ */
  /*         cursor2++; */
  /*         xbt_dynar_free(&s_tokens); */
  /*         continue; */
  /*       } */
  /*       xbt_strbuff_append(clear, (const char *)xbt_dynar_get_as(tokens, cursor2, char *)); */
  /*     } */
  /*     xbt_dynar_free(&s_tokens); */
  /*     cursor2++; */
  /*   } */
  /*   xbt_dynar_free(&tokens); */
  /*   cursor1++; */
  /* } */

  /* xbt_free(frame_name); */

  /* xbt_sha(clear->data, snapshot_hash); */

  /* xbt_strbuff_free(clear); */

}

static xbt_dynar_t MC_get_local_variables_values(void *stack_context){
  
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

  unsigned int cursor = 0;
  dw_variable_t current_variable;
  dw_location_entry_t entry = NULL;
  dw_location_t location_entry = NULL;
  unw_word_t res;
  int frame_found = 0, region_type;
  void *frame_pointer_address = NULL;
  long true_ip, value;
  int stop;

  xbt_dynar_t variables = xbt_dynar_new(sizeof(local_variable_t), local_variable_free_voidp);

  while(ret >= 0 && !stop){

    unw_get_reg(&c, UNW_REG_IP, &ip);
    unw_get_reg(&c, UNW_REG_SP, &sp);

    unw_get_proc_name(&c, frame_name, sizeof (frame_name), &off);

    if(!strcmp(frame_name, "smx_ctx_sysv_wrapper")) /* Stop before context switch with maestro */
      stop = 1;

    if((long)ip > (long)start_text_libsimgrid)
      frame = xbt_dict_get_or_null(mc_local_variables_libsimgrid, frame_name);
    else
      frame = xbt_dict_get_or_null(mc_local_variables_binary, frame_name);

    if(frame == NULL){
      ret = unw_step(&c);
      continue;
    }
    
    true_ip = (long)frame->low_pc + (long)off;
    frame_pointer_address = NULL;

    /* Get frame pointer */
    switch(frame->frame_base->type){
    case e_dw_loclist:
      cursor = 0;
      while(cursor < xbt_dynar_length(frame->frame_base->location.loclist) && !frame_found){
        entry = xbt_dynar_get_as(frame->frame_base->location.loclist, cursor, dw_location_entry_t);
        if((true_ip >= entry->lowpc) && (true_ip < entry->highpc)){
          frame_found = 1;
          switch(entry->location->type){
          case e_dw_compose:
            if(xbt_dynar_length(entry->location->location.compose) > 1){
              frame_pointer_address = NULL; /* TODO : location list with optimizations enabled */
            }else{
              location_entry = xbt_dynar_get_as(entry->location->location.compose, 0, dw_location_t);
              switch(location_entry->type){
              case e_dw_register:
                unw_get_reg(&c, location_entry->location.reg, &res);
                frame_pointer_address = (void*)(long)res;
                break;
              case e_dw_bregister_op:
                unw_get_reg(&c, location_entry->location.breg_op.reg, &res);
                frame_pointer_address = (void*)((long)res + location_entry->location.breg_op.offset);
                break;
              default:
                frame_pointer_address = NULL; /* FIXME : implement other cases (with optimizations enabled) */
                break;
              }
            }
            break;
          default:
            frame_pointer_address = NULL; /* FIXME : implement other cases (with optimizations enabled) */
            break;
          }
        }
        cursor++;
      }
      break;
    default :
      frame_pointer_address = NULL; /* FIXME : implement other cases (with optimizations enabled)*/
      break;
    }

    frame_found = 0;
    cursor = 0;

    xbt_dynar_foreach(frame->variables, cursor, current_variable){
      
      if((long)ip > (long)start_text_libsimgrid)
        region_type = 1;
      else
        region_type = 2;

      local_variable_t new_var = xbt_new0(s_local_variable_t, 1);
      new_var->frame = xbt_strdup(frame_name);
      new_var->ip = (unsigned long)ip;
      new_var->name = xbt_strdup(current_variable->name);
      new_var->type = strdup(current_variable->type_origin);
      new_var->region= region_type;
      
      if(current_variable->address.location != NULL){
        switch(current_variable->address.location->type){
        case e_dw_compose:
          if(xbt_dynar_length(current_variable->address.location->location.compose) > 1){
            /* TODO : location list with optimizations enabled */
          }else{
            location_entry = xbt_dynar_get_as(current_variable->address.location->location.compose, 0, dw_location_t);
            
            switch(location_entry->type){
            case e_dw_register:
              unw_get_reg(&c, location_entry->location.reg, &res);
              value = (long)res;
              break;
            case e_dw_bregister_op:
              unw_get_reg(&c, location_entry->location.breg_op.reg, &res);
              value = (long)res + location_entry->location.breg_op.offset;
              break;
            case e_dw_fbregister_op:
              if(frame_pointer_address != NULL)
                value = (long)((char *)frame_pointer_address + location_entry->location.fbreg_op);
              else
                value = 0;
              break;
            default:
              value = 0; /* FIXME : implement other cases (with optimizations enabled)*/
              break;
            }

            if(value)
              new_var->address = (void *)value;
            else
              new_var->address = NULL;
          }
          break;
        default :
          break;
        }
      }

      xbt_dynar_push(variables, &new_var);

    }

    ret = unw_step(&c);
     
  }

  return variables;

}


static void *MC_get_stack_pointer(void *stack_context, void *heap){

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

static xbt_dynar_t MC_take_snapshot_stacks(mc_snapshot_t *snapshot, void *heap){

  xbt_dynar_t res = xbt_dynar_new(sizeof(s_mc_snapshot_stack_t), MC_snapshot_stack_free_voidp);

  unsigned int cursor = 0;
  stack_region_t current_stack;
  
  xbt_dynar_foreach(stacks_areas, cursor, current_stack){
    mc_snapshot_stack_t st = xbt_new(s_mc_snapshot_stack_t, 1);
    st->local_variables = MC_get_local_variables_values(current_stack->context);
    st->stack_pointer = MC_get_stack_pointer(current_stack->context, heap);
    st->real_address = current_stack->address;
    xbt_dynar_push(res, &st);
    (*snapshot)->stack_sizes = xbt_realloc((*snapshot)->stack_sizes, (cursor + 1) * sizeof(size_t));
    (*snapshot)->stack_sizes[cursor] = current_stack->size - ((char *)st->stack_pointer - (char *)((char *)heap + ((char *)current_stack->address - (char *)std_heap)));
  }

  return res;

}

static xbt_dynar_t MC_take_snapshot_ignore(){
  
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

static void MC_dump_checkpoint_ignore(mc_snapshot_t snapshot){
  
  unsigned int cursor = 0;
  mc_checkpoint_ignore_region_t region;
  size_t offset;
  
  xbt_dynar_foreach(mc_checkpoint_ignore, cursor, region){
    if(region->addr > snapshot->regions[0]->start_addr && (char *)(region->addr) < (char *)snapshot->regions[0]->start_addr + STD_HEAP_SIZE){
      offset = (char *)region->addr - (char *)snapshot->regions[0]->start_addr;
      memset((char *)snapshot->regions[0]->data + offset, 0, region->size);
    }else if(region->addr > snapshot->regions[2]->start_addr && (char *)(region->addr) < (char*)snapshot->regions[2]->start_addr + snapshot->regions[2]->size){
      offset = (char *)region->addr - (char *)snapshot->regions[2]->start_addr;
      memset((char *)snapshot->regions[2]->data + offset, 0, region->size);
    }else if(region->addr > snapshot->regions[1]->start_addr && (char *)(region->addr) < (char*)snapshot->regions[1]->start_addr + snapshot->regions[1]->size){
      offset = (char *)region->addr - (char *)snapshot->regions[1]->start_addr;
      memset((char *)snapshot->regions[1]->data + offset, 0, region->size);
    }
  }

}


mc_snapshot_t MC_take_snapshot(){

  mc_snapshot_t snapshot = xbt_new0(s_mc_snapshot_t, 1);
  snapshot->nb_processes = xbt_swag_size(simix_global->process_list);

  /* Save the std heap and the writable mapped pages of libsimgrid and binary */
  MC_get_memory_regions(snapshot);

  snapshot->to_ignore = MC_take_snapshot_ignore();

  if(_sg_mc_visited > 0 || strcmp(_sg_mc_property_file,"")){
    snapshot->stacks = MC_take_snapshot_stacks(&snapshot, snapshot->regions[0]->data);
    //MC_get_hash_global(snapshot->hash_global, snapshot->regions[1]->data, snapshot->regions[2]->data);
    //MC_get_hash_local(snapshot->hash_local, snapshot->stacks);
  }

  MC_dump_checkpoint_ignore(snapshot);

  return snapshot;

}

void MC_restore_snapshot(mc_snapshot_t snapshot){
  unsigned int i;
  for(i=0; i < NB_REGIONS; i++){
    MC_region_restore(snapshot->regions[i]);
  }

}

mc_snapshot_t SIMIX_pre_mc_snapshot(smx_simcall_t simcall){
  return MC_take_snapshot();
}

void *MC_snapshot(void){
  return simcall_mc_snapshot();
}
