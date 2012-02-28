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

typedef struct surf_storage {
  s_surf_resource_t generic_resource;
  const char* model;
  const char* content;
} s_surf_storage_t, *surf_storage_t;

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

static void* storage_create_resource(const char* id, const char* model,
                                    const char* content, xbt_dict_t storage_properties)
{
    XBT_INFO("SURF: storage_create_resource '%s'",id);
    surf_storage_t storage = NULL;
    xbt_assert(!surf_storage_resource_by_name(id),
                "Storage '%s' declared several times in the platform file",
                id);
    storage = (surf_storage_t) surf_resource_new(sizeof(s_surf_storage_t),
            surf_storage_model, id, storage_properties);
    storage->model = model;
    storage->content = content;
    xbt_lib_set(storage_lib, id, SURF_STORAGE_LEVEL, storage);

    return storage;
}

static void parse_storage_init(sg_platf_storage_cbarg_t storage)
{
  storage_create_resource(storage->id,
      storage->model,
      storage->content,
      storage->properties);
}

static void storage_define_callbacks()
{
  sg_platf_storage_add_cb(parse_storage_init);
}

static void surf_storage_model_init_internal(void)
{
  XBT_DEBUG("surf_storage_model_init_internal");
  surf_storage_model = surf_model_init();

  surf_storage_model->name = "Storage";

  surf_storage_model->extension.storage.open = storage_action_open;
  surf_storage_model->extension.storage.close = storage_action_close;
  surf_storage_model->extension.storage.read = storage_action_read;
  surf_storage_model->extension.storage.write = storage_action_write;
  surf_storage_model->extension.storage.stat = storage_action_stat;
  surf_storage_model->extension.storage.create_resource = storage_create_resource;
}

void surf_storage_model_init_default(void)
{
  surf_storage_model_init_internal();
  storage_define_callbacks();
}
