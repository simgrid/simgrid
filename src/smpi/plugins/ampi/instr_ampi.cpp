/* Copyright (c) 2010-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/smpi/plugins/ampi/instr_ampi.hpp"
#include "smpi/smpi.h"
#include "src/instr/instr_private.hpp"
#include <src/instr/instr_smpi.hpp>
#include <src/smpi/include/smpi_actor.hpp>

static const std::map<std::string, std::string, std::less<>> ampi_colors = {{"migrate", "0.2 0.5 0.2"},
                                                                            {"iteration", "0.5 0.5 0.5"}};

void TRACE_Iteration_in(int rank, simgrid::instr::TIData* extra)
{
  if (not TRACE_smpi_is_enabled()) {
    delete extra;
    return;
  }
  smpi_container(rank)->get_state("MPI_STATE")->add_entity_value("iteration", ampi_colors.at("iteration"));
  smpi_container(rank)->get_state("MPI_STATE")->push_event("iteration", extra);
}

void TRACE_Iteration_out(int rank, simgrid::instr::TIData* extra)
{
  if (not TRACE_smpi_is_enabled()) return;

  smpi_container(rank)->get_state("MPI_STATE")->pop_event(extra);
}

void TRACE_migration_call(int rank, simgrid::instr::TIData* extra)
{
  if (not TRACE_smpi_is_enabled()) return;

  const std::string operation = "migrate";
  if(smpi_process()->replaying()) {//When replaying, we register an event.
    smpi_container(rank)->get_state("MIGRATE_STATE")->add_entity_value(operation);

    auto* type = static_cast<simgrid::instr::EventType*>(smpi_container(rank)->type_->by_name(operation));
    new simgrid::instr::NewEvent(smpi_process()->simulated_elapsed(), smpi_container(rank), type,
                                 type->get_entity_value(operation));
  } else {
    // FIXME From rktesser: Ugly workaround!
    // TI tracing uses states as events, and does not support printing events.
    // So, we need a different code than for replay in order to be able to
    // generate ti_traces for the migration calls.
    if (!TRACE_smpi_is_enabled()) {
      delete extra;
      return;
    }
    smpi_container(rank)->get_state("MIGRATE_STATE")->add_entity_value(operation, ampi_colors.at(operation));
    smpi_container(rank)->get_state("MIGRATE_STATE")->push_event(operation, extra);
    smpi_container(rank)->get_state("MIGRATE_STATE")->pop_event();
  }
}
