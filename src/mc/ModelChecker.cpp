/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/ModelChecker.hpp"
#include "src/mc/explo/Exploration.hpp"
#include "src/mc/explo/LivenessChecker.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_exit.hpp"
#include "src/mc/mc_private.hpp"
#include "src/mc/sosp/RemoteProcessMemory.hpp"
#include "src/mc/transition/TransitionComm.hpp"
#include "xbt/automaton.hpp"
#include "xbt/system_error.hpp"

#include <array>
#include <csignal>
#include <sys/ptrace.h>
#include <sys/wait.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_ModelChecker, mc, "ModelChecker");

::simgrid::mc::ModelChecker* mc_model_checker = nullptr;

namespace simgrid::mc {

} // namespace simgrid::mc
