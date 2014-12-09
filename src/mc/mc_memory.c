/* Copyright (c) 2008-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <sys/stat.h>
#include <fcntl.h>

#include "xbt/log.h"

#include "mc/mc.h"
#include "mc_object_info.h"
#include "mc_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_memory, mc,
                                "Logging specific to MC (memory)");

/* Pointers to each of the heap regions to use */
xbt_mheap_t std_heap = NULL;          /* memory erased each time the MC stuff rollbacks to the beginning. Almost everything goes here */
xbt_mheap_t mc_heap = NULL;           /* memory persistent over the MC rollbacks. Only MC stuff should go there */

/* Initialize the model-checker memory subsystem */
/* It creates the two heap regions: std_heap and mc_heap */
void MC_memory_init()
{
  /* Create the first region HEAP_OFFSET bytes after the heap break address */
  std_heap = mmalloc_get_default_md();
  xbt_assert(std_heap != NULL);

  /* Create the second region a page after the first one ends + safety gap */
  mc_heap =
      xbt_mheap_new_options(-1,
                            (char *) (std_heap) + STD_HEAP_SIZE + xbt_pagesize,
                            0);
  xbt_assert(mc_heap != NULL);
}

/* Finalize the memory subsystem */
#include "xbt_modinter.h"
void MC_memory_exit(void)
{
  if (mc_heap)
    xbt_mheap_destroy(mc_heap);
}
