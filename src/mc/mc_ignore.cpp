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


/**************************** Global variables ******************************/
// Those structures live with the MCer and should be moved in the model_checker
// structure but they are currently used before the MC initialisation
// (in standalone mode).


extern xbt_dynar_t stacks_areas;

/**************************** Structures ******************************/
typedef struct s_mc_stack_ignore_variable {
  char *var_name;
  char *frame;
} s_mc_stack_ignore_variable_t, *mc_stack_ignore_variable_t;

/**************************** Free functions ******************************/

static void stack_ignore_variable_free(mc_stack_ignore_variable_t v)
{
  xbt_free(v->var_name);
  xbt_free(v->frame);
  xbt_free(v);
}

static void stack_ignore_variable_free_voidp(void *v)
{
  stack_ignore_variable_free((mc_stack_ignore_variable_t) * (void **) v);
}

static void checkpoint_ignore_region_free(mc_checkpoint_ignore_region_t r)
{
  xbt_free(r);
}

static void checkpoint_ignore_region_free_voidp(void *r)
{
  checkpoint_ignore_region_free((mc_checkpoint_ignore_region_t) * (void **) r);
}

xbt_dynar_t MC_checkpoint_ignore_new(void)
{
  return xbt_dynar_new(sizeof(mc_checkpoint_ignore_region_t),
                        checkpoint_ignore_region_free_voidp);
}

/***********************************************************************/

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

void MC_process_ignore_memory(mc_process_t process, void *addr, size_t size)
{
  xbt_dynar_t checkpoint_ignore = process->checkpoint_ignore;
  mc_checkpoint_ignore_region_t region =
      xbt_new0(s_mc_checkpoint_ignore_region_t, 1);
  region->addr = addr;
  region->size = size;

  if (xbt_dynar_is_empty(checkpoint_ignore)) {
    xbt_dynar_push(checkpoint_ignore, &region);
  } else {

    unsigned int cursor = 0;
    int start = 0;
    int end = xbt_dynar_length(checkpoint_ignore) - 1;
    mc_checkpoint_ignore_region_t current_region = NULL;

    while (start <= end) {
      cursor = (start + end) / 2;
      current_region =
          (mc_checkpoint_ignore_region_t) xbt_dynar_get_as(checkpoint_ignore,
                                                           cursor,
                                                           mc_checkpoint_ignore_region_t);
      if (current_region->addr == addr) {
        if (current_region->size == size) {
          checkpoint_ignore_region_free(region);
          return;
        } else if (current_region->size < size) {
          start = cursor + 1;
        } else {
          end = cursor - 1;
        }
      } else if (current_region->addr < addr) {
        start = cursor + 1;
      } else {
        end = cursor - 1;
      }
    }

    if (current_region->addr == addr) {
      if (current_region->size < size) {
        xbt_dynar_insert_at(checkpoint_ignore, cursor + 1, &region);
      } else {
        xbt_dynar_insert_at(checkpoint_ignore, cursor, &region);
      }
    } else if (current_region->addr < addr) {
      xbt_dynar_insert_at(checkpoint_ignore, cursor + 1, &region);
    } else {
      xbt_dynar_insert_at(checkpoint_ignore, cursor, &region);
    }
  }
}

}
