/* Copyright (c) 2018. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MC_CONFIG_HPP
#define MC_CONFIG_HPP

/********************************** Configuration of MC **************************************/
extern "C" XBT_PUBLIC int _sg_do_model_check;
extern XBT_PRIVATE int _sg_do_model_check_record;
extern XBT_PRIVATE int _sg_mc_checkpoint;
extern XBT_PUBLIC int _sg_mc_sparse_checkpoint;
extern XBT_PUBLIC int _sg_mc_ksm;
extern XBT_PRIVATE int _sg_mc_timeout;
extern XBT_PRIVATE int _sg_mc_hash;
extern XBT_PRIVATE int _sg_mc_max_depth;
extern "C" XBT_PUBLIC int _sg_mc_max_visited_states;
extern XBT_PUBLIC int _sg_mc_comms_determinism;
extern XBT_PUBLIC int _sg_mc_send_determinism;
extern XBT_PRIVATE int _sg_mc_snapshot_fds;
extern XBT_PRIVATE int _sg_mc_termination;

XBT_PRIVATE void _mc_cfg_cb_reduce(const char* name);
XBT_PRIVATE void _mc_cfg_cb_checkpoint(const char* name);
XBT_PRIVATE void _mc_cfg_cb_sparse_checkpoint(const char* name);
XBT_PRIVATE void _mc_cfg_cb_ksm(const char* name);
XBT_PRIVATE void _mc_cfg_cb_property(const char* name);
XBT_PRIVATE void _mc_cfg_cb_timeout(const char* name);
XBT_PRIVATE void _mc_cfg_cb_snapshot_fds(const char* name);
XBT_PRIVATE void _mc_cfg_cb_hash(const char* name);
XBT_PRIVATE void _mc_cfg_cb_max_depth(const char* name);
XBT_PRIVATE void _mc_cfg_cb_visited(const char* name);
XBT_PRIVATE void _mc_cfg_cb_dot_output(const char* name);
XBT_PRIVATE void _mc_cfg_cb_comms_determinism(const char* name);
XBT_PRIVATE void _mc_cfg_cb_send_determinism(const char* name);
XBT_PRIVATE void _mc_cfg_cb_termination(const char* name);

#endif
