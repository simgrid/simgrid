/* Copyright (c) 2008-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "internal_config.h"
#include "mc_object_info.h"
#include "mc_private.h"
#include "smpi/private.h"
#include "mc/mc_snapshot.h"
#include "mc_ignore.h"
#include "mc_protocol.h"
#include "mc_client.h"

extern "C" {

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_ignore, mc,
                                "Logging specific to MC ignore mechanism");

// ***** Model-checked

void MC_ignore_heap(void *address, size_t size)
{
  if (mc_mode != MC_MODE_CLIENT)
    return;

  s_mc_heap_ignore_region_t region;
  memset(&region, 0, sizeof(region));
  region.address = address;
  region.size = size;
  region.block =
   ((char *) address -
    (char *) std_heap->heapbase) / BLOCKSIZE + 1;
  if (std_heap->heapinfo[region.block].type == 0) {
    region.fragment = -1;
    std_heap->heapinfo[region.block].busy_block.ignore++;
  } else {
    region.fragment =
        ((uintptr_t) (ADDR2UINT(address) % (BLOCKSIZE))) >> std_heap->
        heapinfo[region.block].type;
    std_heap->heapinfo[region.block].busy_frag.ignore[region.fragment]++;
  }

  s_mc_ignore_heap_message_t message;
  message.type = MC_MESSAGE_IGNORE_HEAP;
  message.region = region;
  if (MC_protocol_send(mc_client->fd, &message, sizeof(message)))
    xbt_die("Could not send ignored region to MCer");
}

void MC_remove_ignore_heap(void *address, size_t size)
{
  if (mc_mode != MC_MODE_CLIENT)
    return;

  s_mc_ignore_memory_message_t message;
  message.type = MC_MESSAGE_UNIGNORE_HEAP;
  message.addr = address;
  message.size = size;
  MC_client_send_message(&message, sizeof(message));
}

void MC_ignore_global_variable(const char *name)
{
  // TODO, send a message to the model_checker
  xbt_die("Unimplemented");
}

// *****

extern xbt_dynar_t stacks_areas;

void MC_stack_area_add(stack_region_t stack_area)
{
  if (stacks_areas == NULL)
    stacks_areas = xbt_dynar_new(sizeof(stack_region_t), NULL);
  xbt_dynar_push(stacks_areas, &stack_area);
}

/** @brief Register a stack in the model checker
 *
 *  The stacks are allocated in the heap. The MC handle them especially
 *  when we analyse/compare the content of the heap so it must be told where
 *  they are with this function.
 *
 *  @param stack
 *  @param process Process owning the stack
 *  @param context
 *  @param size    Size of the stack
 */
void MC_new_stack_area(void *stack, smx_process_t process, void *context, size_t size)
{
  xbt_mheap_t heap = mmalloc_set_current_heap(mc_heap);

  stack_region_t region = xbt_new0(s_stack_region_t, 1);
  region->address = stack;
  region->context = context;
  region->size = size;
  region->block =
      ((char *) stack -
       (char *) std_heap->heapbase) / BLOCKSIZE + 1;
#ifdef HAVE_SMPI
  if (smpi_privatize_global_variables && process) {
    region->process_index = smpi_process_index_of_smx_process(process);
  } else
#endif
  region->process_index = -1;

  if (mc_mode == MC_MODE_CLIENT) {
    s_mc_stack_region_message_t message;
    message.type = MC_MESSAGE_STACK_REGION;
    message.stack_region = *region;
    MC_client_send_message(&message, sizeof(message));
  }

  MC_stack_area_add(region);

  mmalloc_set_current_heap(heap);
}

}
