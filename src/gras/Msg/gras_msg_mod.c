/* gras message types handling                                              */

/* Copyright (c) 2003, 2004, 2005, 2006, 2007 Martin Quinson.               */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"
#include "gras/Msg/msg_private.h"
#include "gras/Virtu/virtu_interface.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(gras_msg);

extern xbt_set_t _gras_msgtype_set;

/*
 * Creating procdata for this module
 */
static void *gras_msg_procdata_new(void)
{
  gras_msg_procdata_t res = xbt_new(s_gras_msg_procdata_t, 1);

  res->name = xbt_strdup("gras_msg");
  res->name_len = 0;
  res->msg_queue = xbt_dynar_new(sizeof(s_gras_msg_t), NULL);
  res->msg_waitqueue = xbt_dynar_new(sizeof(s_gras_msg_t), NULL);
  res->cbl_list = xbt_dynar_new(sizeof(gras_cblist_t *), gras_cbl_free);
  res->timers = xbt_dynar_new(sizeof(s_gras_timer_t), NULL);
  res->msg_to_receive_queue = xbt_fifo_new();
  res->msg_to_receive_queue_meas = xbt_fifo_new();
  res->msg_received = xbt_queue_new(0, sizeof(s_gras_msg_t));

  return (void *) res;
}

/*
 * Freeing procdata for this module
 */
static void gras_msg_procdata_free(void *data)
{
  gras_msg_procdata_t res = (gras_msg_procdata_t) data;

  xbt_dynar_free(&(res->msg_queue));
  xbt_dynar_free(&(res->msg_waitqueue));
  xbt_dynar_free(&(res->cbl_list));
  xbt_dynar_free(&(res->timers));
  xbt_fifo_free(res->msg_to_receive_queue);
  xbt_fifo_free(res->msg_to_receive_queue_meas);

  free(res->name);
  free(res);
}

/*
 * Module registration
 */
int gras_msg_libdata_id;
void gras_msg_register()
{
  gras_msg_libdata_id =
    gras_procdata_add("gras_msg", gras_msg_procdata_new,
                      gras_msg_procdata_free);
}

/*
 * Initialize this submodule.
 */
void gras_msg_init(void)
{
  /* only initialize once */
  if (_gras_msgtype_set != NULL)
    return;

  VERB0("Initializing Msg");

  _gras_msgtype_set = xbt_set_new();

  memcpy(_GRAS_header, "GRAS", 4);
  _GRAS_header[4] = GRAS_PROTOCOL_VERSION;
  _GRAS_header[5] = (char) GRAS_THISARCH;

  gras_msg_ctx_mallocator =
    xbt_mallocator_new(1000,
                       gras_msg_ctx_mallocator_new_f,
                       gras_msg_ctx_mallocator_free_f,
                       gras_msg_ctx_mallocator_reset_f);
}

/*
 * Finalize the msg module
 */
void gras_msg_exit(void)
{
  VERB0("Exiting Msg");
  xbt_set_free(&_gras_msgtype_set);

  xbt_mallocator_free(gras_msg_ctx_mallocator);
}
