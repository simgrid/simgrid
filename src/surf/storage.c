/* Copyright (c) 2004 - 2013. The SimGrid Team.
 * All rights reserved.                                                                 */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package.             */

#include "xbt/ex.h"
#include "xbt/dict.h"
#include "portable.h"
#include "surf_private.h"
#include "storage_private.h"
#include "surf/surf_resource.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_storage, surf,
                                "Logging specific to the SURF storage module");

xbt_lib_t storage_lib;
int ROUTING_STORAGE_LEVEL;      //Routing for storagelevel
int ROUTING_STORAGE_HOST_LEVEL;
int SURF_STORAGE_LEVEL;
xbt_lib_t storage_type_lib;
int ROUTING_STORAGE_TYPE_LEVEL; //Routing for storage_type level

xbt_dynar_t mount_list = NULL;  /* temporary store of current mount storage */

surf_model_t surf_storage_model = NULL;
lmm_system_t storage_maxmin_system = NULL;
static int storage_selective_update = 0;
static xbt_swag_t
    storage_running_action_set_that_does_not_need_being_checked = NULL;

static xbt_dynar_t storage_list;

#define GENERIC_LMM_ACTION(action) action->generic_lmm_action
#define GENERIC_ACTION(action) GENERIC_LMM_ACTION(action).generic_action

static xbt_dict_t parse_storage_content(char *filename, size_t *used_size);
static int storage_action_unref(surf_action_t action);
static void storage_action_state_set(surf_action_t action, e_surf_action_state_t state);
static surf_action_t storage_action_execute (void *storage, size_t size, e_surf_action_storage_type_t type);

static surf_action_t storage_action_ls(void *storage, const char* path)
{
  surf_action_t action = storage_action_execute(storage,0, LS);
  action->ls_dict = NULL;
  xbt_dict_t ls_dict = xbt_dict_new();

  char* key;
  size_t size = 0;
  xbt_dict_cursor_t cursor = NULL;

  xbt_dynar_t dyn = NULL;
  char* file = NULL;

  // for each file in the storage content
  xbt_dict_foreach(((storage_t)storage)->content,cursor,key,size){
    // Search if file start with the prefix 'path'
    if(xbt_str_start_with(key,path)){
      file = &key[strlen(path)];

      // Split file with '/'
      dyn = xbt_str_split(file,"/");
      file = xbt_dynar_get_as(dyn,0,char*);

      // file
      if(xbt_dynar_length(dyn) == 1){
        xbt_dict_set(ls_dict,file,&size,NULL);
      }
      // Directory
      else
      {
        // if directory does not exist yet in the dictionary
        if(!xbt_dict_get_or_null(ls_dict,file))
          xbt_dict_set(ls_dict,file,NULL,NULL);
      }
      xbt_dynar_free(&dyn);
    }
  }

  action->ls_dict = ls_dict;
  return action;
}

static surf_action_t storage_action_open(void *storage, const char* mount,
                                         const char* path)
{
  XBT_DEBUG("\tOpen file '%s'",path);
  xbt_dict_t content_dict = ((storage_t)storage)->content;
  size_t size = (size_t) xbt_dict_get_or_null(content_dict,path);
  // if file does not exist create an empty file
  if(!size){
    xbt_dict_set(content_dict,path,&size,NULL);
    XBT_DEBUG("File '%s' was not found, file created.",path);
  }
  surf_file_t file = xbt_new0(s_surf_file_t,1);
  file->name = xbt_strdup(path);
  file->size = size;
  file->mount = xbt_strdup(mount);

  surf_action_t action = storage_action_execute(storage,0, OPEN);
  action->file = (void *)file;
  return action;
}

static surf_action_t storage_action_close(void *storage, surf_file_t fd)
{
  char *filename = fd->name;
  XBT_DEBUG("\tClose file '%s' size '%zu'",filename,fd->size);
  // unref write actions from storage
  surf_action_storage_t write_action;
  unsigned int i;
  xbt_dynar_foreach(((storage_t)storage)->write_actions,i,write_action) {
    if ((write_action->generic_lmm_action.generic_action.file) == fd) {
      xbt_dynar_cursor_rm(((storage_t)storage)->write_actions, &i);
      storage_action_unref((surf_action_t) write_action);
    }
  }

  free(fd->name);
  free(fd->mount);
  xbt_free(fd);
  surf_action_t action = storage_action_execute(storage,0, CLOSE);
  return action;
}

static surf_action_t storage_action_read(void *storage, size_t size,
                                         surf_file_t fd)
{
  if(size > fd->size)
    size = fd->size;
  surf_action_t action = storage_action_execute(storage,size,READ);
  return action;
}

static surf_action_t storage_action_write(void *storage, size_t size, 
                                          surf_file_t fd)
{
  char *filename = fd->name;
  XBT_DEBUG("\tWrite file '%s' size '%zu/%zu'",filename,size,fd->size);

  surf_action_t action = storage_action_execute(storage,size,WRITE);
  action->file = fd;

  // If the storage is full
  if(((storage_t)storage)->used_size==((storage_t)storage)->size) {
    storage_action_state_set((surf_action_t) action, SURF_ACTION_FAILED);
  }
  return action;
}

static surf_action_t storage_action_execute (void *storage, size_t size, e_surf_action_storage_type_t type)
{
  surf_action_storage_t action = NULL;
  storage_t STORAGE = storage;

  XBT_IN("(%s,%zu", surf_resource_name(STORAGE), size);
  action =
      surf_action_new(sizeof(s_surf_action_storage_t), size, surf_storage_model,
          STORAGE->state_current != SURF_RESOURCE_ON);

  // Save the storage on action
  action->storage = storage;
  GENERIC_LMM_ACTION(action).suspended = 0;     /* Should be useless because of the
                                                   calloc but it seems to help valgrind... */

  GENERIC_LMM_ACTION(action).variable =
      lmm_variable_new(storage_maxmin_system, action, 1.0, -1.0 , 3);

  // Must be less than the max bandwidth for all actions
  lmm_expand(storage_maxmin_system, STORAGE->constraint,
             GENERIC_LMM_ACTION(action).variable, 1.0);

  switch(type) {
  case OPEN:
  case CLOSE:
  case STAT:
  case LS:
    break;
  case READ:
    lmm_expand(storage_maxmin_system, STORAGE->constraint_read,
               GENERIC_LMM_ACTION(action).variable, 1.0);
    break;
  case WRITE:
    lmm_expand(storage_maxmin_system, STORAGE->constraint_write,
               GENERIC_LMM_ACTION(action).variable, 1.0);
    xbt_dynar_push(((storage_t)storage)->write_actions,&action);
    surf_action_ref((surf_action_t) action);
    break;
  }
  action->type = type;
  XBT_OUT();
  return (surf_action_t) action;
}

static xbt_dict_t storage_get_properties(const void *storage)
{
  return surf_resource_properties(surf_storage_resource_priv(storage));
}

static xbt_dict_t storage_get_content(void *storage)
{
  /* For the moment this action has no cost, but in the future we could take in account access latency of the disk */
  /*surf_action_t action = storage_action_execute(storage,0, LS);*/

  void *st = surf_storage_resource_priv(storage);
  xbt_dict_t content_dict = xbt_dict_new();
  xbt_dict_cursor_t cursor = NULL;
  char *file;
  size_t size;
  xbt_dict_foreach(((storage_t)st)->content, cursor, file, size){
    xbt_dict_set(content_dict,file,&size,NULL);
  }
  return content_dict;
}

static void* storage_create_resource(const char* id, const char* model,
    const char* type_id, const char* content_name, const char* content_type, xbt_dict_t properties){
  storage_t storage = NULL;

  xbt_assert(!surf_storage_resource_priv(surf_storage_resource_by_name(id)),
              "Storage '%s' declared several times in the platform file",
              id);
  storage = (storage_t) surf_resource_new(sizeof(s_storage_t),
          surf_storage_model, id, properties, NULL);

  storage->state_current = SURF_RESOURCE_ON;
  storage->used_size = 0;
  storage->size = 0;
  storage->write_actions = xbt_dynar_new(sizeof(char *),NULL);

  storage_type_t storage_type = xbt_lib_get_or_null(storage_type_lib, type_id,ROUTING_STORAGE_TYPE_LEVEL);
  double Bread =
      surf_parse_get_bandwidth(xbt_dict_get(storage_type->properties,"Bread"));
  double Bwrite =
      surf_parse_get_bandwidth(xbt_dict_get(storage_type->properties,"Bwrite"));
  double Bconnection =
      surf_parse_get_bandwidth(xbt_dict_get(storage_type->properties,
                                            "Bconnection"));
  XBT_DEBUG("Create resource with Bconnection '%f' Bread '%f' Bwrite '%f' and Size '%lu'",Bconnection,Bread,Bwrite,(unsigned long)storage_type->size);
  storage->constraint       = lmm_constraint_new(storage_maxmin_system, storage, Bconnection);
  storage->constraint_read  = lmm_constraint_new(storage_maxmin_system, storage, Bread);
  storage->constraint_write = lmm_constraint_new(storage_maxmin_system, storage, Bwrite);
  storage->content = parse_storage_content((char*)content_name,&(storage->used_size));
  storage->content_type = xbt_strdup(content_type);
  storage->size = storage_type->size;
  storage->type_id = xbt_strdup(type_id);

  xbt_lib_set(storage_lib, id, SURF_STORAGE_LEVEL, storage);

  XBT_DEBUG("SURF storage create resource\n\t\tid '%s'\n\t\ttype '%s' \n\t\tmodel '%s' \n\t\tproperties '%p'\n\t\tBread '%f'\n",
      id,
      model,
      type_id,
      storage_type->properties,
      Bread);

  if (!storage_list)
    storage_list = xbt_dynar_new(sizeof(char *),NULL);
  xbt_dynar_push(storage_list,&storage);

  return storage;
}

static void storage_finalize(void)
{
  lmm_system_free(storage_maxmin_system);
  storage_maxmin_system = NULL;

  surf_model_exit(surf_storage_model);
  surf_storage_model = NULL;

  xbt_dynar_free(&storage_list);

  xbt_swag_free
      (storage_running_action_set_that_does_not_need_being_checked);
  storage_running_action_set_that_does_not_need_being_checked = NULL;
}

static void storage_update_actions_state(double now, double delta)
{
  surf_action_storage_t action = NULL;
  surf_action_storage_t next_action = NULL;
  xbt_swag_t running_actions = surf_storage_model->states.running_action_set;

  // Update the disk usage
  // Update the file size
  // For each action of type write
  xbt_swag_foreach_safe(action, next_action, running_actions) {
    if(action->type == WRITE)
    {
      double rate = lmm_variable_getvalue(GENERIC_LMM_ACTION(action).variable);
      /* Hack to avoid rounding differences between x86 and x86_64
       * (note that the next sizes are of type size_t). */
      long incr = delta * rate + MAXMIN_PRECISION;
      ((storage_t)(action->storage))->used_size += incr; // disk usage
      ((surf_action_t)action)->file->size += incr; // file size
    }
  }

  xbt_swag_foreach_safe(action, next_action, running_actions) {

    double_update(&(GENERIC_ACTION(action).remains),
                  lmm_variable_getvalue(GENERIC_LMM_ACTION(action).variable) * delta);

    if (GENERIC_LMM_ACTION(action).generic_action.max_duration != NO_MAX_DURATION)
      double_update(&(GENERIC_ACTION(action).max_duration), delta);

    if(GENERIC_ACTION(action).remains > 0 &&
        lmm_get_variable_weight(GENERIC_LMM_ACTION(action).variable) > 0 &&
        ((storage_t)action->storage)->used_size == ((storage_t)action->storage)->size)
    {
      GENERIC_ACTION(action).finish = surf_get_clock();
      storage_action_state_set((surf_action_t) action, SURF_ACTION_FAILED);
    } else if ((GENERIC_ACTION(action).remains <= 0) &&
        (lmm_get_variable_weight(GENERIC_LMM_ACTION(action).variable) > 0))
    {
      GENERIC_ACTION(action).finish = surf_get_clock();
      storage_action_state_set((surf_action_t) action, SURF_ACTION_DONE);
    } else if ((GENERIC_ACTION(action).max_duration != NO_MAX_DURATION) &&
               (GENERIC_ACTION(action).max_duration <= 0))
    {
      GENERIC_ACTION(action).finish = surf_get_clock();
      storage_action_state_set((surf_action_t) action, SURF_ACTION_DONE);
    }
  }

  return;
}

static double storage_share_resources(double NOW)
{
  XBT_DEBUG("storage_share_resources %f",NOW);
  s_surf_action_storage_t action;
  unsigned int i,j;
  storage_t storage;
  surf_action_storage_t write_action;

  double min_completion = generic_maxmin_share_resources(surf_storage_model->states.running_action_set,
      xbt_swag_offset(action, generic_lmm_action.variable),
      storage_maxmin_system, lmm_solve);

  double rate;
  // Foreach disk
  xbt_dynar_foreach(storage_list,i,storage)
  {
    rate = 0;
    // Foreach write action on disk
    xbt_dynar_foreach(storage->write_actions,j,write_action)
    {
      rate += lmm_variable_getvalue(write_action->generic_lmm_action.variable);
    }
    if(rate > 0)
      min_completion = MIN(min_completion, (storage->size-storage->used_size)/rate);
  }

  return min_completion;
}

static int storage_resource_used(void *resource_id)
{
  THROW_UNIMPLEMENTED;
  return 0;
}

static void storage_resources_state(void *id, tmgr_trace_event_t event_type,
                                 double value, double time)
{
  THROW_UNIMPLEMENTED;
}

static int storage_action_unref(surf_action_t action)
{
  action->refcount--;
  if (!action->refcount) {
    xbt_swag_remove(action, action->state_set);
    if (((surf_action_lmm_t) action)->variable)
      lmm_variable_free(storage_maxmin_system,
                        ((surf_action_lmm_t) action)->variable);
#ifdef HAVE_TRACING
    xbt_free(action->category);
#endif
    surf_action_free(&action);
    return 1;
  }
  return 0;
}

static void storage_action_cancel(surf_action_t action)
{
  surf_action_state_set(action, SURF_ACTION_FAILED);
  return;
}

static void storage_action_state_set(surf_action_t action, e_surf_action_state_t state)
{
  surf_action_state_set(action, state);
  return;
}

static void storage_action_suspend(surf_action_t action)
{
  XBT_IN("(%p)", action);
  if (((surf_action_lmm_t) action)->suspended != 2) {
    lmm_update_variable_weight(storage_maxmin_system,
                               ((surf_action_lmm_t) action)->variable,
                               0.0);
    ((surf_action_lmm_t) action)->suspended = 1;
  }
  XBT_OUT();
}

static void storage_action_resume(surf_action_t action)
{
  THROW_UNIMPLEMENTED;
}

static int storage_action_is_suspended(surf_action_t action)
{
  return (((surf_action_lmm_t) action)->suspended == 1);
}

static void storage_action_set_max_duration(surf_action_t action, double duration)
{
  THROW_UNIMPLEMENTED;
}

static void storage_action_set_priority(surf_action_t action, double priority)
{
  THROW_UNIMPLEMENTED;
}

static void parse_storage_init(sg_platf_storage_cbarg_t storage)
{
  void* stype = xbt_lib_get_or_null(storage_type_lib,
                                    storage->type_id,
                                    ROUTING_STORAGE_TYPE_LEVEL);
  if(!stype) xbt_die("No storage type '%s'",storage->type_id);

  // if storage content is not specified use the content of storage_type if exist
  if(!strcmp(storage->content,"") && strcmp(((storage_type_t) stype)->content,"")){
    storage->content = ((storage_type_t) stype)->content;
    storage->content_type = ((storage_type_t) stype)->content_type;
    XBT_DEBUG("For disk '%s' content is empty, inherit the content (of type %s) from storage type '%s' ",
        storage->id,((storage_type_t) stype)->content_type,
        ((storage_type_t) stype)->type_id);
  }

  XBT_DEBUG("SURF storage create resource\n\t\tid '%s'\n\t\ttype '%s' "
      "\n\t\tmodel '%s' \n\t\tcontent '%s'\n\t\tcontent_type '%s' "
      "\n\t\tproperties '%p'\n",
      storage->id,
      ((storage_type_t) stype)->model,
      ((storage_type_t) stype)->type_id,
      storage->content,
      storage->content_type,
      ((storage_type_t) stype)->properties);

  storage_create_resource(storage->id,
     ((storage_type_t) stype)->model,
     ((storage_type_t) stype)->type_id,
     storage->content,
     storage->content_type,
     storage->properties);
}

static void parse_mstorage_init(sg_platf_mstorage_cbarg_t mstorage)
{
  XBT_DEBUG("parse_mstorage_init");
}

static void parse_storage_type_init(sg_platf_storage_type_cbarg_t storagetype_)
{
  XBT_DEBUG("parse_storage_type_init");
}

static void parse_mount_init(sg_platf_mount_cbarg_t mount)
{
  XBT_DEBUG("parse_mount_init");
}

static void storage_define_callbacks()
{
  sg_platf_storage_add_cb(parse_storage_init);
  sg_platf_storage_type_add_cb(parse_storage_type_init);
  sg_platf_mstorage_add_cb(parse_mstorage_init);
  sg_platf_mount_add_cb(parse_mount_init);
}

static void surf_storage_model_init_internal(void)
{
  s_surf_action_t action;

  XBT_DEBUG("surf_storage_model_init_internal");
  surf_storage_model = surf_model_init();

  storage_running_action_set_that_does_not_need_being_checked =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));

  surf_storage_model->name = "Storage";
  surf_storage_model->action_unref = storage_action_unref;
  surf_storage_model->action_cancel = storage_action_cancel;
  surf_storage_model->action_state_set = storage_action_state_set;

  surf_storage_model->model_private->finalize = storage_finalize;
  surf_storage_model->model_private->update_actions_state = storage_update_actions_state;
  surf_storage_model->model_private->share_resources = storage_share_resources;
  surf_storage_model->model_private->resource_used = storage_resource_used;
  surf_storage_model->model_private->update_resource_state = storage_resources_state;

  surf_storage_model->suspend = storage_action_suspend;
  surf_storage_model->resume = storage_action_resume;
  surf_storage_model->is_suspended = storage_action_is_suspended;
  surf_storage_model->set_max_duration = storage_action_set_max_duration;
  surf_storage_model->set_priority = storage_action_set_priority;

  surf_storage_model->extension.storage.open = storage_action_open;
  surf_storage_model->extension.storage.close = storage_action_close;
  surf_storage_model->extension.storage.read = storage_action_read;
  surf_storage_model->extension.storage.write = storage_action_write;
  surf_storage_model->extension.storage.ls = storage_action_ls;
  surf_storage_model->extension.storage.get_properties = storage_get_properties;
  surf_storage_model->extension.storage.get_content = storage_get_content;

  if (!storage_maxmin_system) {
    storage_maxmin_system = lmm_system_new(storage_selective_update);
  }

}

void surf_storage_model_init_default(void)
{
  surf_storage_model_init_internal();
  storage_define_callbacks();

  xbt_dynar_push(model_list, &surf_storage_model);
}

static void storage_parse_storage(sg_platf_storage_cbarg_t storage)
{
  xbt_assert(!xbt_lib_get_or_null(storage_lib, storage->id,ROUTING_STORAGE_LEVEL),
               "Reading a storage, processing unit \"%s\" already exists", storage->id);

  // Verification of an existing type_id
#ifndef NDEBUG
  void* storage_type = xbt_lib_get_or_null(storage_type_lib, storage->type_id,ROUTING_STORAGE_TYPE_LEVEL);
#endif
  xbt_assert(storage_type,"Reading a storage, type id \"%s\" does not exists", storage->type_id);

  XBT_DEBUG("ROUTING Create a storage name '%s' with type_id '%s' and content '%s'",
      storage->id,
      storage->type_id,
      storage->content);

  xbt_lib_set(storage_lib,
      storage->id,
      ROUTING_STORAGE_LEVEL,
      (void *) xbt_strdup(storage->type_id));
}

static xbt_dict_t parse_storage_content(char *filename, size_t *used_size)
{
  *used_size = 0;
  if ((!filename) || (strcmp(filename, "") == 0))
    return NULL;

  xbt_dict_t parse_content = xbt_dict_new_homogeneous(NULL);
  FILE *file = NULL;

  file = surf_fopen(filename, "r");
  xbt_assert(file != NULL, "Cannot open file '%s' (path=%s)", filename,
              xbt_str_join(surf_path, ":"));

  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  char path[1024];
  size_t size;


  while ((read = xbt_getline(&line, &len, file)) != -1) {
    if (read){
    if(sscanf(line,"%s %zu",path, &size)==2) {
        *used_size += size;
        xbt_dict_set(parse_content,path,(void*) size,NULL);
      } else {
        xbt_die("Be sure of passing a good format for content file.\n");
      }
    }
  }
  free(line);
  fclose(file);
  return parse_content;
}

static void storage_parse_storage_type(sg_platf_storage_type_cbarg_t storage_type)
{
  xbt_assert(!xbt_lib_get_or_null(storage_type_lib, storage_type->id,ROUTING_STORAGE_TYPE_LEVEL),
               "Reading a storage type, processing unit \"%s\" already exists", storage_type->id);

  storage_type_t stype = xbt_new0(s_storage_type_t, 1);
  stype->model = xbt_strdup(storage_type->model);
  stype->properties = storage_type->properties;
  stype->content = xbt_strdup(storage_type->content);
  stype->content_type = xbt_strdup(storage_type->content_type);
  stype->type_id = xbt_strdup(storage_type->id);
  stype->size = storage_type->size;

  XBT_DEBUG("ROUTING Create a storage type id '%s' with model '%s', "
      "content '%s', and content_type '%s'",
      stype->type_id,
      stype->model,
      storage_type->content,
      storage_type->content_type);

  xbt_lib_set(storage_type_lib,
      stype->type_id,
      ROUTING_STORAGE_TYPE_LEVEL,
      (void *) stype);
}
static void storage_parse_mstorage(sg_platf_mstorage_cbarg_t mstorage)
{
  THROW_UNIMPLEMENTED;
//  mount_t mnt = xbt_new0(s_mount_t, 1);
//  mnt->id = xbt_strdup(mstorage->type_id);
//  mnt->name = xbt_strdup(mstorage->name);
//
//  if(!mount_list){
//    XBT_DEBUG("Creata a Mount list for %s",A_surfxml_host_id);
//    mount_list = xbt_dynar_new(sizeof(char *), NULL);
//  }
//  xbt_dynar_push(mount_list,(void *) mnt);
//  free(mnt->id);
//  free(mnt->name);
//  xbt_free(mnt);
//  XBT_DEBUG("ROUTING Mount a storage name '%s' with type_id '%s'",mstorage->name, mstorage->id);
}

static void mount_free(void *p)
{
  mount_t mnt = p;
  xbt_free(mnt->name);
}

static void storage_parse_mount(sg_platf_mount_cbarg_t mount)
{
  // Verification of an existing storage
#ifndef NDEBUG
  void* storage = xbt_lib_get_or_null(storage_lib, mount->storageId,ROUTING_STORAGE_LEVEL);
#endif
  xbt_assert(storage,"Disk id \"%s\" does not exists", mount->storageId);

  XBT_DEBUG("ROUTING Mount '%s' on '%s'",mount->storageId, mount->name);

  s_mount_t mnt;
  mnt.storage =
    surf_storage_resource_priv(surf_storage_resource_by_name(mount->storageId));
  mnt.name = xbt_strdup(mount->name);

  if(!mount_list){
    XBT_DEBUG("Create a Mount list for %s",A_surfxml_host_id);
    mount_list = xbt_dynar_new(sizeof(s_mount_t), mount_free);
  }
  xbt_dynar_push(mount_list,&mnt);
}

static XBT_INLINE void routing_storage_type_free(void *r)
{
  storage_type_t stype = r;
  free(stype->model);
  free(stype->type_id);
  free(stype->content);
  free(stype->content_type);
  xbt_dict_free(&(stype->properties));
  free(stype);
}

static XBT_INLINE void surf_storage_resource_free(void *r)
{
  // specific to storage
  storage_t storage = r;
  xbt_dict_free(&storage->content);
  xbt_dynar_free(&storage->write_actions);
  free(storage->type_id);
  free(storage->content_type);
  // generic resource
  surf_resource_free(r);
}

static XBT_INLINE void routing_storage_host_free(void *r)
{
  xbt_dynar_t dyn = r;
  xbt_dynar_free(&dyn);
}

void storage_register_callbacks() {

  ROUTING_STORAGE_LEVEL = xbt_lib_add_level(storage_lib,xbt_free);
  ROUTING_STORAGE_HOST_LEVEL = xbt_lib_add_level(storage_lib,routing_storage_host_free);
  ROUTING_STORAGE_TYPE_LEVEL = xbt_lib_add_level(storage_type_lib,routing_storage_type_free);
  SURF_STORAGE_LEVEL = xbt_lib_add_level(storage_lib,surf_storage_resource_free);

  sg_platf_storage_add_cb(storage_parse_storage);
  sg_platf_mstorage_add_cb(storage_parse_mstorage);
  sg_platf_storage_type_add_cb(storage_parse_storage_type);
  sg_platf_mount_add_cb(storage_parse_mount);
}

