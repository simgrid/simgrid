/* Copyright (c) 2014-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** \file
 *  This file contains the MC replay/record functionnality.
 *  A MC path may be recorded by using ``-cfg=model-check/record:1`'`.
 *  The path is written in the log output and an be replayed with MC disabled
 *  (even with an non-LC build) with `--cfg=model-check/replay:$replayPath`.
 *
 *  The same version of Simgrid should be used and the same arguments should be
 *  passed to the application (without the MC specific arguments).
 */

#ifndef SIMGRID_MC_RECORD_H
#define SIMGRID_MC_RECORD_H

#include <xbt/base.h>
#include <xbt/dynar.h>
#include <xbt/fifo.h>

SG_BEGIN_DECL()

/** Whether the MC record mode is enabled
 *
 *  The behaviour is not changed. The only real difference is that
 *  the path is writtent in the log when an interesting path is found.
 */
#define MC_record_is_active() _sg_do_model_check_record

// **** Data conversion

/** An element in the recorded path
 *
 *  At each decision point, we need to record which process transition
 *  is trigerred and potentially which value is associated with this
 *  transition. The value is used to find which communication is triggerred
 *  in things like waitany and for associating a given value of MC_random()
 *  calls.
 */
typedef struct s_mc_record_item {
  int pid;
  int value;
} s_mc_record_item_t, *mc_record_item_t;

/** Convert a string representation of the path into a array of `s_mc_record_item_t`
 */
XBT_PRIVATE xbt_dynar_t MC_record_from_string(const char* data);

/** Generate a string representation
*
* The current format is a ";"-delimited list of pairs:
* "pid0,value0;pid2,value2;pid3,value3". The value can be
* omitted is it is null.
*/
XBT_PRIVATE char* MC_record_stack_to_string(xbt_fifo_t stack);

/** Dump the path represented by a given stack in the log
 */
XBT_PRIVATE void MC_record_dump_path(xbt_fifo_t stack);

// ***** Replay

/** Replay a path represented by the record items
 *
 *  \param start Array of record item
 *  \item  count Number of record items
 */
XBT_PRIVATE void MC_record_replay(mc_record_item_t start, size_t count);

/** Replay a path represented by a string
 *
 *  \param data String representation of the path
 */
XBT_PRIVATE void MC_record_replay_from_string(const char* data);

XBT_PRIVATE void MC_record_replay_init(void);

SG_END_DECL()

#endif
