/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "internal_config.h"
#include "mc_object_info.h"
#include "mc/mc_private.h"
#include "smpi/private.h"
#include "mc/mc_snapshot.h"
#include "mc/mc_ignore.h"
#include "mc/mc_protocol.h"
#include "mc/mc_client.h"

extern "C" {

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mcer_ignore, mc,
                                "Logging specific to MC ignore mechanism");

extern xbt_dynar_t mc_heap_comparison_ignore;

void MC_heap_region_ignore_insert(mc_heap_ignore_region_t region)
{
  if (mc_heap_comparison_ignore == NULL) {
    mc_heap_comparison_ignore =
        xbt_dynar_new(sizeof(mc_heap_ignore_region_t),
                      heap_ignore_region_free_voidp);
    xbt_dynar_push(mc_heap_comparison_ignore, &region);
    return;
  }

  unsigned int cursor = 0;
  mc_heap_ignore_region_t current_region = NULL;
  int start = 0;
  int end = xbt_dynar_length(mc_heap_comparison_ignore) - 1;

  // Find the position where we want to insert the mc_heap_ignore_region_t:
  while (start <= end) {
    cursor = (start + end) / 2;
    current_region =
        (mc_heap_ignore_region_t) xbt_dynar_get_as(mc_heap_comparison_ignore,
                                                   cursor,
                                                   mc_heap_ignore_region_t);
    if (current_region->address == region->address) {
      heap_ignore_region_free(region);
      return;
    } else if (current_region->address < region->address) {
      start = cursor + 1;
    } else {
      end = cursor - 1;
    }
  }

  // Insert it mc_heap_ignore_region_t:
  if (current_region->address < region->address)
    xbt_dynar_insert_at(mc_heap_comparison_ignore, cursor + 1, &region);
  else
    xbt_dynar_insert_at(mc_heap_comparison_ignore, cursor, &region);
}

void MC_heap_region_ignore_remove(void *address, size_t size)
{
  unsigned int cursor = 0;
  int start = 0;
  int end = xbt_dynar_length(mc_heap_comparison_ignore) - 1;
  mc_heap_ignore_region_t region;
  int ignore_found = 0;

  while (start <= end) {
    cursor = (start + end) / 2;
    region =
        (mc_heap_ignore_region_t) xbt_dynar_get_as(mc_heap_comparison_ignore,
                                                   cursor,
                                                   mc_heap_ignore_region_t);
    if (region->address == address) {
      ignore_found = 1;
      break;
    } else if (region->address < address) {
      start = cursor + 1;
    } else {
      if ((char *) region->address <= ((char *) address + size)) {
        ignore_found = 1;
        break;
      } else {
        end = cursor - 1;
      }
    }
  }

  if (ignore_found == 1) {
    xbt_dynar_remove_at(mc_heap_comparison_ignore, cursor, NULL);
    MC_remove_ignore_heap(address, size);
  }
}

}
