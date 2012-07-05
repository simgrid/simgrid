/* Copyright (c) 2008-2012 Da SimGrid Team. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <libgen.h>
#include "mc_private.h"
#include "xbt/module.h"


XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_checkpoint, mc,
                                "Logging specific to mc_checkpoint");

static mc_mem_region_t MC_region_new(int type, void *start_addr, size_t size);
static void MC_region_restore(mc_mem_region_t reg);
static void MC_region_destroy(mc_mem_region_t reg);

static void MC_snapshot_add_region(mc_snapshot_t snapshot, int type, void *start_addr, size_t size);

static mc_mem_region_t MC_region_new(int type, void *start_addr, size_t size)
{
  mc_mem_region_t new_reg = xbt_new0(s_mc_mem_region_t, 1);
  new_reg->type = type;
  new_reg->start_addr = start_addr;
  new_reg->size = size;
  new_reg->data = xbt_malloc0(size);
  XBT_DEBUG("New reg data %p, start_addr %p, size %zu", new_reg->data, start_addr, size);
  memcpy(new_reg->data, start_addr, size);
  
  return new_reg;
}

static void MC_region_restore(mc_mem_region_t reg)
{
  /*FIXME: check if start_addr is still mapped, if it is not, then map it
    before copying the data */
 
  XBT_DEBUG("Memcpy : dest %p, src %p, size %zu", reg->start_addr, reg->data, reg->size);
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
  switch(type){
  case 0 : 
    XBT_DEBUG("New region heap (%zu)", size);
    break;
  case 1 : 
    XBT_DEBUG("New region libsimgrid (%zu)", size);
    break;
  case 2 : 
    XBT_DEBUG("New region program data (%zu)", size);
    break;
  }
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

  /* FIXME: free the memory map */
}

void MC_take_snapshot_liveness(mc_snapshot_t snapshot)
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
        } else {
          if (!memcmp(basename(maps->regions[i].pathname), basename(xbt_binary_name), strlen(basename(xbt_binary_name)))){
            MC_snapshot_add_region(snapshot, 2, reg.start_addr, (char*)reg.end_addr - (char*)reg.start_addr);
          } 
        }
      }
    }
    i++;
  }

  /* FIXME: free the memory map */
}

void MC_restore_snapshot(mc_snapshot_t snapshot)
{
  unsigned int i;
  for(i=0; i < snapshot->num_reg; i++){
    MC_region_restore(snapshot->regions[i]);
    switch(snapshot->regions[i]->type){
    case 0 : 
      XBT_DEBUG("heap restored");
      break;
    case 1:
      XBT_DEBUG("libsimgrid (data) restored");
      break;
    case 2:
      XBT_DEBUG("data program restored");
      break;
    }

  }

}

void MC_free_snapshot(mc_snapshot_t snapshot)
{
  unsigned int i;
  for(i=0; i < snapshot->num_reg; i++)
    MC_region_destroy(snapshot->regions[i]);

  xbt_free(snapshot);
}

static int data_program_region_compare(void *d1, void *d2, size_t size);
static int data_libsimgrid_region_compare(void *d1, void *d2, size_t size);

static int data_program_region_compare(void *d1, void *d2, size_t size){
  int distance = 0;
  int i;
  
  for(i=0; i<size; i++){
    if(memcmp(((char *)d1) + i, ((char *)d2) + i, 1) != 0){
      fprintf(stderr,"Different byte (offset=%d) (%p - %p) in data program region\n", i, (char *)d1 + i, (char *)d2 + i);
      distance++;
    }
  }
  
  fprintf(stderr, "Hamming distance between data program regions : %d\n", distance);

  return distance;
}

int data_libsimgrid_region_compare(void *d1, void *d2, size_t size){
  int distance = 0;
  int i;
  
  for(i=0; i<size; i++){
    if(memcmp(((char *)d1) + i, ((char *)d2) + i, 1) != 0){
      fprintf(stderr, "Different byte (offset=%d) (%p - %p) in data libsimgrid region\n", i, (char *)d1 + i, (char *)d2 + i);
      distance++;
    }
  }
  
  fprintf(stderr, "Hamming distance between data libsimgrid regions : %d\n", distance);
  
  return distance;
}

int snapshot_compare(mc_snapshot_t s1, mc_snapshot_t s2){

  
  if(s1->num_reg != s2->num_reg){
    XBT_INFO("Different num_reg (s1 = %u, s2 = %u)", s1->num_reg, s2->num_reg);
    return 1;
  }

  int i;
  int errors = 0;

  for(i=0 ; i< s1->num_reg ; i++){

    if(s1->regions[i]->type != s2->regions[i]->type){
      XBT_INFO("Different type of region");
      errors++;
    }

    switch(s1->regions[i]->type){
    case 0:
      if(s1->regions[i]->size != s2->regions[i]->size){
        XBT_INFO("Different size of heap (s1 = %zu, s2 = %zu)", s1->regions[i]->size, s2->regions[i]->size);
        errors++;
      }
      if(s1->regions[i]->start_addr != s2->regions[i]->start_addr){
        XBT_INFO("Different start addr of heap (s1 = %p, s2 = %p)", s1->regions[i]->start_addr, s2->regions[i]->start_addr);
        errors++;
      }
      if(mmalloc_compare_heap(s1->regions[i]->data, s2->regions[i]->data)){
        XBT_INFO("Different heap (mmalloc_compare)");
        errors++; 
      }
      break;
    case 1 :
      if(s1->regions[i]->size != s2->regions[i]->size){
        XBT_INFO("Different size of libsimgrid (s1 = %zu, s2 = %zu)", s1->regions[i]->size, s2->regions[i]->size);
        errors++;
      }
      if(s1->regions[i]->start_addr != s2->regions[i]->start_addr){
        XBT_INFO("Different start addr of libsimgrid (s1 = %p, s2 = %p)", s1->regions[i]->start_addr, s2->regions[i]->start_addr);
        errors++;
      }
      if(data_libsimgrid_region_compare(s1->regions[i]->data, s2->regions[i]->data, s1->regions[i]->size) != 0){
        XBT_INFO("Different memcmp for data in libsimgrid");
        errors++;
      }
      break;
    case 2 :
      if(s1->regions[i]->size != s2->regions[i]->size){
        XBT_INFO("Different size of data program (s1 = %zu, s2 = %zu)", s1->regions[i]->size, s2->regions[i]->size);
        errors++;
      }
      if(s1->regions[i]->start_addr != s2->regions[i]->start_addr){
        XBT_INFO("Different start addr of data program (s1 = %p, s2 = %p)", s1->regions[i]->start_addr, s2->regions[i]->start_addr);
        errors++;
      }
      if(data_program_region_compare(s1->regions[i]->data, s2->regions[i]->data, s1->regions[i]->size) != 0){
        XBT_INFO("Different memcmp for data in program");
        errors++;
      }
      break;
    default:
      break;
    }
  }

  return (errors > 0);
  
}

