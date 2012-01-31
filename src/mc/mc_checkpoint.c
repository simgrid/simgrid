/* Copyright (c) 2008-2012 Da SimGrid Team. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <libgen.h>
#include "mc_private.h"


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
  if(reg->type == 3){
    memory_map_t maps = get_memory_map();
    MC_UNSET_RAW_MEM;
    unsigned int i=0;
    s_map_region_t r;
    while(i < maps->mapsize){
      r = maps->regions[i];
      if (maps->regions[i].pathname != NULL){
	if (!memcmp(maps->regions[i].pathname, "[stack]", 7)){
	  size_t diff = (char*)reg->start_addr - (char*)r.start_addr;
	  void *segment = malloc(reg->size + diff);
	  XBT_DEBUG("Size of segment : %zu", sizeof(segment));
	  memcpy((char *)segment + diff, reg->data, reg->size);
	  memcpy(r.start_addr, segment, sizeof(segment));
	  XBT_DEBUG("Memcpy region ok");
	  break;
	}
      }
      i++;
    }
  }else{
    XBT_DEBUG("Memcpy : dest %p, src %p, size %zu", reg->start_addr, reg->data, reg->size);
    memcpy(reg->start_addr, reg->data, reg->size);
  }
  
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

  for(i=0; i< snapshot->num_reg; i++){
    MC_region_destroy(snapshot->regions[i]);
  }

  snapshot->num_reg = 0;

  i = 0;

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
	  if (!memcmp(basename(maps->regions[i].pathname), basename(prog_name), strlen(basename(prog_name)))){
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


