/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/internal_config.h"
#include "mc_object_info.h"
#include "mc_private.h"
#include "src/smpi/private.h"
#include "src/mc/mc_snapshot.h"
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

  xbt_mheap_t heap = mmalloc_get_current_heap();

  s_mc_ignore_heap_message_t message;
  message.type = MC_MESSAGE_IGNORE_HEAP;
  message.address = address;
  message.size = size;
  message.block =
   ((char *) address -
    (char *) heap->heapbase) / BLOCKSIZE + 1;
  if (heap->heapinfo[message.block].type == 0) {
    message.fragment = -1;
    heap->heapinfo[message.block].busy_block.ignore++;
  } else {
    message.fragment =
        ((uintptr_t) (ADDR2UINT(address) % (BLOCKSIZE))) >>
        heap->heapinfo[message.block].type;
    heap->heapinfo[message.block].busy_frag.ignore[message.fragment]++;
  }

  if (MC_protocol_send(mc_client->fd, &message, sizeof(message)))
    xbt_die("Could not send ignored region to MCer");
}

void MC_remove_ignore_heap(void *address, size_t size)
{
  if (mc_mode != MC_MODE_CLIENT)
    return;

  s_mc_ignore_memory_message_t message;
  message.type = MC_MESSAGE_UNIGNORE_HEAP;
  message.addr = (std::uintptr_t) address;
  message.size = size;
  MC_client_send_message(&message, sizeof(message));
}

void MC_ignore_global_variable(const char *name)
{
  // TODO, send a message to the model_checker
  xbt_die("Unimplemented");
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
void MC_register_stack_area(void *stack, smx_process_t process, void *context, size_t size)
{
  if (mc_mode != MC_MODE_CLIENT)
    return;

  xbt_mheap_t heap = mmalloc_get_current_heap();

  s_stack_region_t region;
  memset(&region, 0, sizeof(region));
  region.address = stack;
  region.context = context;
  region.size = size;
  region.block =
      ((char *) stack -
       (char *) heap->heapbase) / BLOCKSIZE + 1;
#ifdef HAVE_SMPI
  if (smpi_privatize_global_variables && process) {
    region.process_index = smpi_process_index_of_smx_process(process);
  } else
#endif
  region.process_index = -1;

  s_mc_stack_region_message_t message;
  message.type = MC_MESSAGE_STACK_REGION;
  message.stack_region = region;
  MC_client_send_message(&message, sizeof(message));
}

}
