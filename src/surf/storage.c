/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012. The SimGrid Team.
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

surf_model_t surf_storage_model = NULL;
lmm_system_t storage_maxmin_system = NULL;
static int storage_selective_update = 0;
static xbt_swag_t
    storage_running_action_set_that_does_not_need_being_checked = NULL;

#define GENERIC_LMM_ACTION(action) action->generic_lmm_action
#define GENERIC_ACTION(action) GENERIC_LMM_ACTION(action).generic_action

typedef struct surf_storage {
  s_surf_resource_t generic_resource;
  const char* type;
  const char* content; /*should be a dict */
} s_surf_storage_t, *surf_storage_t;

static void storage_action_state_set(surf_action_t action, e_surf_action_state_t state);
static surf_action_t storage_action_sleep (void *storage, double duration);

static surf_action_t storage_action_open(void *storage, const char* path, const char* mode)
{
  return storage_action_sleep(storage,1.0);
}

static surf_action_t storage_action_close(void *storage, surf_file_t fp)
{
  return storage_action_sleep(storage,2.0);
}

static surf_action_t storage_action_read(void *storage, void* ptr, size_t size, size_t nmemb, surf_file_t stream)
{
  return storage_action_sleep(storage,3.0);
}

static surf_action_t storage_action_write(void *storage, const void* ptr, size_t size, size_t nmemb, surf_file_t stream)
{
  return storage_action_sleep(storage,4.0);
}

static surf_action_t storage_action_stat(void *storage, int fd, void* buf)
{
  return storage_action_sleep(storage,5.0);
}

static surf_action_t storage_action_execute (void *storage, double size)
{
  surf_action_storage_t action = NULL;
  storage_t STORAGE = storage;

  XBT_IN("(%s,%g)", surf_resource_name(STORAGE), size);
  action =
      surf_action_new(sizeof(s_surf_action_storage_t), size, surf_storage_model,
          STORAGE->state_current != SURF_RESOURCE_ON);

  GENERIC_LMM_ACTION(action).suspended = 0;     /* Should be useless because of the
                                                   calloc but it seems to help valgrind... */
  GENERIC_LMM_ACTION(action).variable =
      lmm_variable_new(storage_maxmin_system, action, 1.0, -1.0 , 1);
  XBT_OUT();
  return (surf_action_t) action;
}

static surf_action_t storage_action_sleep (void *storage, double duration)
{
  surf_action_storage_t action = NULL;

  if (duration > 0)
    duration = MAX(duration, MAXMIN_PRECISION);

  XBT_IN("(%s,%g)", surf_resource_name(storage), duration);
  action = (surf_action_storage_t) storage_action_execute(storage, 1.0);
  GENERIC_ACTION(action).max_duration = duration;
  GENERIC_LMM_ACTION(action).suspended = 2;
  if (duration == NO_MAX_DURATION) {
    /* Move to the *end* of the corresponding action set. This convention
       is used to speed up update_resource_state  */
    xbt_swag_remove(action, ((surf_action_t) action)->state_set);
    ((surf_action_t) action)->state_set =
        storage_running_action_set_that_does_not_need_being_checked;
    xbt_swag_insert(action, ((surf_action_t) action)->state_set);
  }
  XBT_OUT();
  return (surf_action_t) action;
}

static void* storage_create_resource(const char* id, const char* model,const char* type_id,
                                    const char* content, xbt_dict_t storage_properties)
{
  storage_t storage = NULL;

  xbt_assert(!surf_storage_resource_by_name(id),
              "Storage '%s' declared several times in the platform file",
              id);
  storage = (storage_t) surf_resource_new(sizeof(s_storage_t),
          surf_storage_model, id,storage_properties);

  storage->state_current = SURF_RESOURCE_ON;

  xbt_lib_set(storage_lib, id, SURF_STORAGE_LEVEL, storage);

  XBT_DEBUG("SURF storage create resource\n\t\tid '%s'\n\t\ttype '%s' \n\t\tmodel '%s' \n\t\tcontent '%s'\n\t\tproperties '%p'\n",
      id,
      model,
      type_id,
      content,
      storage_properties);

  return storage;
}

static void storage_finalize(void)
{
  lmm_system_free(storage_maxmin_system);
  storage_maxmin_system = NULL;

  surf_model_exit(surf_storage_model);
  surf_storage_model = NULL;

  xbt_swag_free
      (storage_running_action_set_that_does_not_need_being_checked);
  storage_running_action_set_that_does_not_need_being_checked = NULL;
}

static void storage_update_actions_state(double now, double delta)
{
  surf_action_storage_t action = NULL;
  surf_action_storage_t next_action = NULL;
  xbt_swag_t running_actions = surf_storage_model->states.running_action_set;
  xbt_swag_foreach_safe(action, next_action, running_actions) {

    double_update(&(GENERIC_ACTION(action).remains),
                  lmm_variable_getvalue(GENERIC_LMM_ACTION(action).variable) * delta);
    if (GENERIC_LMM_ACTION(action).generic_action.max_duration != NO_MAX_DURATION)
      double_update(&(GENERIC_ACTION(action).max_duration), delta);
    if ((GENERIC_ACTION(action).remains <= 0) &&
        (lmm_get_variable_weight(GENERIC_LMM_ACTION(action).variable) > 0)) {
      GENERIC_ACTION(action).finish = surf_get_clock();
      storage_action_state_set((surf_action_t) action, SURF_ACTION_DONE);
    } else if ((GENERIC_ACTION(action).max_duration != NO_MAX_DURATION) &&
               (GENERIC_ACTION(action).max_duration <= 0)) {
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
  return generic_maxmin_share_resources(surf_storage_model->states.running_action_set,
      xbt_swag_offset(action, generic_lmm_action.variable),
      storage_maxmin_system, lmm_solve);
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
//  XBT_INFO("SURF storage create resource\n\t\tid '%s'\n\t\ttype '%s' \n\t\tmodel '%s' \n\t\tcontent '%s'\n\t\tproperties '%p'\n",
//      storage->id,
//      ((storage_type_t) stype)->model,
//      ((storage_type_t) stype)->type_id,
//      ((storage_type_t) stype)->content,
//      ((storage_type_t) stype)->properties);

 storage_create_resource(storage->id,
     ((storage_type_t) stype)->model,
     ((storage_type_t) stype)->type_id,
     ((storage_type_t) stype)->content,
     NULL);
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
  surf_storage_model->extension.storage.stat = storage_action_stat;
  surf_storage_model->extension.storage.create_resource = storage_create_resource;

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
