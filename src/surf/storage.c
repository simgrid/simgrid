/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012. The SimGrid Team.
 * All rights reserved.                                                                 */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package.             */

#include "xbt/ex.h"
#include "xbt/dict.h"
#include "portable.h"
#include "surf_private.h"
#include "surf/surf_resource.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_storage, surf,
                                "Logging specific to the SURF storage module");

surf_model_t surf_storage_model = NULL;

static surf_action_t storage_action_open(void *workstation, const char* path, const char* mode)
{
  return NULL;
}

static surf_action_t storage_action_close(void *workstation, surf_file_t fp)
{
  return NULL;
}

static surf_action_t storage_action_read(void *workstation, void* ptr, size_t size, size_t nmemb, surf_file_t stream)
{
  return NULL;
}

static surf_action_t storage_action_write(void *workstation, const void* ptr, size_t size, size_t nmemb, surf_file_t stream)
{
  return NULL;
}

static surf_action_t storage_action_stat(void *workstation, int fd, void* buf)
{
  return NULL;
}

static void surf_storage_model_init_internal(void)
{
  surf_storage_model = surf_model_init();

  surf_storage_model->name = "Storage";

  surf_storage_model->extension.workstation.open = storage_action_open;
  surf_storage_model->extension.workstation.close = storage_action_close;
  surf_storage_model->extension.workstation.read = storage_action_read;
  surf_storage_model->extension.workstation.write = storage_action_write;
  surf_storage_model->extension.workstation.stat = storage_action_stat;
}
