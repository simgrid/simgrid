/* Copyright (c) 2018-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MC_CONFIG_HPP
#define MC_CONFIG_HPP

#include <xbt/config.hpp>

/********************************** Configuration of MC **************************************/
namespace simgrid::mc {
XBT_DECLARE_ENUM_CLASS(ReductionMode, none, dpor, sdpor, odpor);
XBT_DECLARE_ENUM_CLASS(ModelCheckingMode, NONE, APP_SIDE, CHECKER_SIDE, REPLAY);
ReductionMode get_model_checking_reduction(); // "model-check/reduction" == "DPOR"
XBT_PUBLIC ModelCheckingMode get_model_checking_mode();
XBT_PUBLIC void set_model_checking_mode(ModelCheckingMode mode);
}; // namespace simgrid::mc

extern XBT_PUBLIC simgrid::config::Flag<std::string> _sg_mc_buffering;
extern XBT_PRIVATE simgrid::config::Flag<int> _sg_mc_checkpoint;
extern XBT_PUBLIC simgrid::config::Flag<std::string> _sg_mc_property_file;
extern XBT_PUBLIC simgrid::config::Flag<bool> _sg_mc_comms_determinism;
extern XBT_PUBLIC simgrid::config::Flag<bool> _sg_mc_send_determinism;
extern XBT_PUBLIC simgrid::config::Flag<bool> _sg_mc_unfolding_checker;
extern XBT_PRIVATE simgrid::config::Flag<bool> _sg_mc_timeout;
extern XBT_PRIVATE simgrid::config::Flag<int> _sg_mc_max_depth;
extern XBT_PRIVATE simgrid::config::Flag<int> _sg_mc_random_seed;
extern XBT_PRIVATE simgrid::config::Flag<std::string> _sg_mc_dot_output_file;
extern XBT_PUBLIC simgrid::config::Flag<std::string> _sg_mc_strategy;

#endif
