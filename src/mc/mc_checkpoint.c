/* Copyright (c) 2008-2012 Da SimGrid Team. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <libgen.h>
#include "mc_private.h"
#include "xbt/module.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_checkpoint, mc,
                                "Logging specific to mc_checkpoint");

void *start_text_libsimgrid;
void *start_plt, *end_plt;
char *libsimgrid_path;

static mc_mem_region_t MC_region_new(int type, void *start_addr, size_t size);
static void MC_region_restore(mc_mem_region_t reg);
static void MC_region_destroy(mc_mem_region_t reg);

static void MC_snapshot_add_region(mc_snapshot_t snapshot, int type, void *start_addr, size_t size);

static int data_program_region_compare(void *d1, void *d2, size_t size);
static int data_libsimgrid_region_compare(void *d1, void *d2, size_t size);

static mc_mem_region_t MC_region_new(int type, void *start_addr, size_t size)
{
  mc_mem_region_t new_reg = xbt_new0(s_mc_mem_region_t, 1);
  new_reg->type = type;
  new_reg->start_addr = start_addr;
  new_reg->size = size;
  new_reg->data = xbt_malloc0(size);
  memcpy(new_reg->data, start_addr, size);

  XBT_DEBUG("New region : type : %d, data : %p, size : %zu", type, new_reg->data, size);
  
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
  snapshot->num_reg++;
  return;
} 

void MC_take_snapshot(mc_snapshot_t snapshot)
{
  unsigned int i = 0;
  s_map_region_t reg;
  memory_map_t maps = get_memory_map();

  /* Save the std heap and the writable mapped pages of libsimgrid */
  while (i < maps->mapsize) {
    reg = maps->regions[i];
    if ((reg.prot & PROT_WRITE)){
      if (maps->regions[i].pathname == NULL){
        if (reg.start_addr == std_heap){ // only save the std heap (and not the raw one)
          MC_snapshot_add_region(snapshot, 0, reg.start_addr, (char*)reg.end_addr - (char*)reg.start_addr);
        }
      } else {
        if (!memcmp(basename(maps->regions[i].pathname), "libsimgrid", 10)){
          MC_snapshot_add_region(snapshot, 1, reg.start_addr, (char*)reg.end_addr - (char*)reg.start_addr);
        } 
      }
    }
    i++;
  }

  free_memory_map(maps);
}

void MC_take_snapshot_liveness(mc_snapshot_t snapshot)
{
  unsigned int i = 0;
  s_map_region_t reg;
  memory_map_t maps = get_memory_map();
  int nb_reg = 0;

  /* Save the std heap and the writable mapped pages of libsimgrid */
  while (i < maps->mapsize && nb_reg < 3) {
    reg = maps->regions[i];
    if ((reg.prot & PROT_WRITE)){
      if (maps->regions[i].pathname == NULL){
        if (reg.start_addr == std_heap){ // only save the std heap (and not the raw one)
          MC_snapshot_add_region(snapshot, 0, reg.start_addr, (char*)reg.end_addr - (char*)reg.start_addr);
          nb_reg++;
        }
      } else {
        if (!memcmp(basename(maps->regions[i].pathname), "libsimgrid", 10)){
          MC_snapshot_add_region(snapshot, 1, reg.start_addr, (char*)reg.end_addr - (char*)reg.start_addr);
          nb_reg++;
        } else {
          if (!memcmp(basename(maps->regions[i].pathname), basename(xbt_binary_name), strlen(basename(xbt_binary_name)))){
            MC_snapshot_add_region(snapshot, 2, reg.start_addr, (char*)reg.end_addr - (char*)reg.start_addr);
            nb_reg++;
          }
        }
      }
    }else if ((reg.prot & PROT_READ)){
      if (maps->regions[i].pathname != NULL){
        if (!memcmp(basename(maps->regions[i].pathname), "libsimgrid", 10)){
          start_text_libsimgrid = reg.start_addr;
          libsimgrid_path = strdup(maps->regions[i].pathname);
        }
      }
    }
    i++;
  }
  
  free_memory_map(maps);

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

  xbt_free(snapshot);
}

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
        distance++;
      }
    }
  }
  
  XBT_DEBUG("Hamming distance between data libsimgrid regions : %d", distance); fflush(NULL);
  
  return distance;
}

int snapshot_compare(mc_snapshot_t s1, mc_snapshot_t s2){

  int errors = 0, i;
  
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
        errors++;
      }
      if(s1->regions[i]->start_addr != s2->regions[i]->start_addr){
        XBT_DEBUG("Different start addr of heap (s1 = %p, s2 = %p)", s1->regions[i]->start_addr, s2->regions[i]->start_addr);
        errors++;
      }
      if(mmalloc_compare_heap((xbt_mheap_t)s1->regions[i]->data, (xbt_mheap_t)s2->regions[i]->data)){
        XBT_DEBUG("Different heap (mmalloc_compare)");
        errors++; 
      }
      break;
    case 1 :
      /* Compare data libsimgrid region */
      if(s1->regions[i]->size != s2->regions[i]->size){
        XBT_DEBUG("Different size of libsimgrid (data) (s1 = %zu, s2 = %zu)", s1->regions[i]->size, s2->regions[i]->size);
        errors++;
      }
      if(s1->regions[i]->start_addr != s2->regions[i]->start_addr){
        XBT_DEBUG("Different start addr of libsimgrid (data) (s1 = %p, s2 = %p)", s1->regions[i]->start_addr, s2->regions[i]->start_addr);
        errors++;
      }
      if(data_libsimgrid_region_compare(s1->regions[i]->data, s2->regions[i]->data, s1->regions[i]->size) != 0){
        XBT_DEBUG("Different memcmp for data in libsimgrid");
        errors++;
      }
      break;

    case 2 :
       /* Compare data program region */
      if(s1->regions[i]->size != s2->regions[i]->size){
        XBT_DEBUG("Different size of data program (s1 = %zu, s2 = %zu)", s1->regions[i]->size, s2->regions[i]->size);
        //errors++;
        return 1;
      }
      if(s1->regions[i]->start_addr != s2->regions[i]->start_addr){
        XBT_DEBUG("Different start addr of data program (s1 = %p, s2 = %p)", s1->regions[i]->start_addr, s2->regions[i]->start_addr);
        //errors++;
        return 1;
      }
      if(data_program_region_compare(s1->regions[i]->data, s2->regions[i]->data, s1->regions[i]->size) != 0){
        XBT_DEBUG("Different memcmp for data in program");
        //errors++;
        return 1;
      }
      break;
 
    }

  }


  return errors > 0;
  
}

void get_plt_section(){

  FILE *fp;
  char *line = NULL;            /* Temporal storage for each line that is readed */
  ssize_t read;                 /* Number of bytes readed */
  size_t n = 0;                 /* Amount of bytes to read by getline */

  char *lfields[7];
  int i, plt_not_found = 1;
  unsigned long int size, offset;

  char *command = bprintf( "objdump --section-headers %s", libsimgrid_path);

  fp = popen(command, "r");

  if(fp == NULL)
    perror("popen failed");

  while ((read = getline(&line, &n, fp)) != -1 && plt_not_found == 1) {

    if(n == 0)
      continue;

     /* Wipeout the new line character */
    line[read - 1] = '\0';

    lfields[0] = strtok(line, " ");

    if(lfields[0] == NULL)
      continue;

    if(strcmp(lfields[0], "Sections:") == 0 || strcmp(lfields[0], "Idx") == 0 || strcmp(lfields[0], "libsimgrid.so:") == 0)
      continue;

    for (i = 1; i < 7 && lfields[i - 1] != NULL; i++) {
      lfields[i] = strtok(NULL, " ");
    }

    if(i>=5){
      if(strcmp(lfields[1], ".plt") == 0){
        size = strtoul(lfields[2], NULL, 16);
        offset = strtoul(lfields[4], NULL, 16);
        start_plt = (char *)start_text_libsimgrid + offset;
        end_plt = (char *)start_plt + size;
        fprintf(stderr, ".plt section : %p - %p \n", start_plt, end_plt); 
        plt_not_found = 0;
      }
    }
    
    
  }

  free(command);
  free(line);
  pclose(fp);

}

