/* Copyright (c) 2008-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc_model_checker.h"
#include "mc_page_store.h"

mc_model_checker_t mc_model_checker = NULL;

mc_model_checker_t MC_model_checker_new(pid_t pid, int socket)
{
  mc_model_checker_t mc = xbt_new0(s_mc_model_checker_t, 1);
  mc->pages = mc_pages_store_new();
  mc->fd_clear_refs = -1;
  mc->fd_pagemap = -1;
  MC_process_init(&mc->process, pid, socket);
  mc->hosts = xbt_dict_new();
  return mc;
}

void MC_model_checker_delete(mc_model_checker_t mc)
{
  mc_pages_store_delete(mc->pages);
  if(mc->record)
    xbt_dynar_free(&mc->record);
  MC_process_clear(&mc->process);
  xbt_dict_free(&mc->hosts);
}

unsigned long MC_smx_get_maxpid(void)
{
  if (mc_mode == MC_MODE_STANDALONE)
    return simix_process_maxpid;

  unsigned long maxpid;
  MC_process_read_variable(&mc_model_checker->process, "simix_process_maxpid",
    &maxpid, sizeof(maxpid));
  return maxpid;
}
