/* Copyright (c) 2015-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/Exploration.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_exit.hpp"
#include "src/simgrid/sg_config.hpp"

#if HAVE_SMPI
#include "smpi/smpi.h"
#endif

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(mc);

using namespace simgrid::mc;

int main(int argc, char** argv)
{
  xbt_assert(argc >= 2, "Missing arguments");

  // Currently, we need this before sg_config_init:
  simgrid::mc::set_model_checking_mode(simgrid::mc::ModelCheckingMode::CHECKER_SIDE);

  // The initialization function can touch argv.
  // We make a copy of argv before modifying it in order to pass the original value to the model-checked application:
  std::vector<char*> argv_copy{argv, argv + argc + 1};

  xbt_log_init(&argc, argv);
#if HAVE_SMPI
  smpi_init_options(); // that's OK to call it twice, and we need it ASAP
#endif
  sg_config_init(&argc, argv);

  std::unique_ptr<Exploration> explo;

  if (_sg_mc_comms_determinism || _sg_mc_send_determinism)
    explo = std::unique_ptr<Exploration>(
        create_communication_determinism_checker(argv_copy, get_model_checking_reduction()));
  else if (_sg_mc_unfolding_checker)
    explo = std::unique_ptr<Exploration>(create_udpor_checker(argv_copy));
  else
    explo = std::unique_ptr<Exploration>(create_dfs_exploration(argv_copy, get_model_checking_reduction()));

  ExitStatus status;
  try {
    explo->run();
    status = ExitStatus::SUCCESS;
  } catch (const McError& e) {
    status = e.value;
  }
  return static_cast<int>(status);
}
