/* Copyright (c) 2008-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/config.h"
#include "xbt/log.h"
#include <xbt/sysdep.h>

#include "src/mc/mc_replay.hpp"
#include <mc/mc.h>

#include <simgrid/sg_config.h>

#if SIMGRID_HAVE_MC
#include "src/mc/mc_private.hpp"
#include "src/mc/mc_safety.hpp"
#endif

#include "src/mc/mc_record.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_config, mc, "Configuration of the Model Checker");

#if SIMGRID_HAVE_MC
namespace simgrid {
namespace mc {
/* Configuration support */
simgrid::mc::ReductionMode reduction_mode = simgrid::mc::ReductionMode::unset;
}
}
#endif

#if !SIMGRID_HAVE_MC
#define _sg_do_model_check 0
#endif

int _sg_mc_timeout = 0;

void _mc_cfg_cb_timeout(const char *name)
{
  if (_sg_cfg_init_status && not(_sg_do_model_check || not MC_record_path.empty()))
    xbt_die("You are specifying a value to enable/disable timeout for wait requests after the initialization (through MSG_config?), but model-checking was not activated at config time (through bu the program was not runned under the model-checker (with simgrid-mc)). This won't work, sorry.");

  _sg_mc_timeout = xbt_cfg_get_boolean(name);
}

#if SIMGRID_HAVE_MC
int _sg_do_model_check = 0;
int _sg_do_model_check_record = 0;
int _sg_mc_checkpoint = 0;
int _sg_mc_sparse_checkpoint = 0;
int _sg_mc_ksm = 0;
std::string _sg_mc_property_file;
int _sg_mc_hash = 0;
int _sg_mc_max_depth = 1000;
int _sg_mc_max_visited_states = 0;
std::string _sg_mc_dot_output_file;
int _sg_mc_comms_determinism = 0;
int _sg_mc_send_determinism = 0;
int _sg_mc_snapshot_fds = 0;
int _sg_mc_termination = 0;

void _mc_cfg_cb_reduce(const char *name)
{
  if (_sg_cfg_init_status && not _sg_do_model_check)
    xbt_die
        ("You are specifying a reduction strategy after the initialization (through MSG_config?), but model-checking was not activated at config time (through bu the program was not runned under the model-checker (with simgrid-mc)). This won't work, sorry.");

  std::string val = xbt_cfg_get_string(name);
  if (val == "none")
    simgrid::mc::reduction_mode = simgrid::mc::ReductionMode::none;
  else if (val == "dpor")
    simgrid::mc::reduction_mode = simgrid::mc::ReductionMode::dpor;
  else
    xbt_die("configuration option %s can only take 'none' or 'dpor' as a value", name);
}

void _mc_cfg_cb_checkpoint(const char *name)
{
  if (_sg_cfg_init_status && not _sg_do_model_check)
    xbt_die
        ("You are specifying a checkpointing value after the initialization (through MSG_config?), but model-checking was not activated at config time (through bu the program was not runned under the model-checker (with simgrid-mc)). This won't work, sorry.");

  _sg_mc_checkpoint = xbt_cfg_get_int(name);
}

void _mc_cfg_cb_sparse_checkpoint(const char *name) {
  if (_sg_cfg_init_status && not _sg_do_model_check)
    xbt_die("You are specifying a checkpointing value after the initialization (through MSG_config?), but model-checking was not activated at config time (through bu the program was not runned under the model-checker (with simgrid-mc)). This won't work, sorry.");

  _sg_mc_sparse_checkpoint = xbt_cfg_get_boolean(name);
}

void _mc_cfg_cb_ksm(const char *name)
{
  if (_sg_cfg_init_status && not _sg_do_model_check)
    xbt_die("You are specifying a KSM value after the initialization (through MSG_config?), but model-checking was not activated at config time (through --cfg=model-check:1). This won't work, sorry.");

  _sg_mc_ksm = xbt_cfg_get_boolean(name);
}

void _mc_cfg_cb_property(const char *name)
{
  if (_sg_cfg_init_status && not _sg_do_model_check)
    xbt_die
        ("You are specifying a property after the initialization (through MSG_config?), but model-checking was not activated at config time (through bu the program was not runned under the model-checker (with simgrid-mc)). This won't work, sorry.");

  _sg_mc_property_file = xbt_cfg_get_string(name);
}

void _mc_cfg_cb_hash(const char *name)
{
  if (_sg_cfg_init_status && not _sg_do_model_check)
    xbt_die
        ("You are specifying a value to enable/disable the use of global hash to speedup state comparaison, but model-checking was not activated at config time (through bu the program was not runned under the model-checker (with simgrid-mc)). This won't work, sorry.");

  _sg_mc_hash = xbt_cfg_get_boolean(name);
}

void _mc_cfg_cb_snapshot_fds(const char *name)
{
  if (_sg_cfg_init_status && not _sg_do_model_check)
    xbt_die
        ("You are specifying a value to enable/disable the use of FD snapshotting, but model-checking was not activated at config time (through bu the program was not runned under the model-checker (with simgrid-mc)). This won't work, sorry.");

  _sg_mc_snapshot_fds = xbt_cfg_get_boolean(name);
}

void _mc_cfg_cb_max_depth(const char *name)
{
  if (_sg_cfg_init_status && not _sg_do_model_check)
    xbt_die
        ("You are specifying a max depth value after the initialization (through MSG_config?), but model-checking was not activated at config time (through bu the program was not runned under the model-checker (with simgrid-mc)). This won't work, sorry.");

  _sg_mc_max_depth = xbt_cfg_get_int(name);
}

void _mc_cfg_cb_visited(const char *name)
{
  if (_sg_cfg_init_status && not _sg_do_model_check)
    xbt_die
        ("You are specifying a number of stored visited states after the initialization (through MSG_config?), but model-checking was not activated at config time (through bu the program was not runned under the model-checker (with simgrid-mc)). This won't work, sorry.");

  _sg_mc_max_visited_states = xbt_cfg_get_int(name);
}

void _mc_cfg_cb_dot_output(const char *name)
{
  if (_sg_cfg_init_status && not _sg_do_model_check)
    xbt_die
        ("You are specifying a file name for a dot output of graph state after the initialization (through MSG_config?), but model-checking was not activated at config time (through bu the program was not runned under the model-checker (with simgrid-mc)). This won't work, sorry.");

  _sg_mc_dot_output_file = xbt_cfg_get_string(name);
}

void _mc_cfg_cb_comms_determinism(const char *name)
{
  if (_sg_cfg_init_status && not _sg_do_model_check)
    xbt_die
        ("You are specifying a value to enable/disable the detection of determinism in the communications schemes after the initialization (through MSG_config?), but model-checking was not activated at config time (through bu the program was not runned under the model-checker (with simgrid-mc)). This won't work, sorry.");

  _sg_mc_comms_determinism = xbt_cfg_get_boolean(name);
}

void _mc_cfg_cb_send_determinism(const char *name)
{
  if (_sg_cfg_init_status && not _sg_do_model_check)
    xbt_die
        ("You are specifying a value to enable/disable the detection of send-determinism in the communications schemes after the initialization (through MSG_config?), but model-checking was not activated at config time (through bu the program was not runned under the model-checker (with simgrid-mc)). This won't work, sorry.");

  _sg_mc_send_determinism = xbt_cfg_get_boolean(name);
}

void _mc_cfg_cb_termination(const char *name)
{
  if (_sg_cfg_init_status && not _sg_do_model_check)
    xbt_die
        ("You are specifying a value to enable/disable the detection of non progressive cycles after the initialization (through MSG_config?), but model-checking was not activated at config time (through bu the program was not runned under the model-checker (with simgrid-mc)). This won't work, sorry.");

  _sg_mc_termination = xbt_cfg_get_boolean(name);
}

#endif
