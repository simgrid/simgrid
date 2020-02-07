/* Copyright (c) 2018-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INCLUDE_SIMGRID_ENGINE_H_
#define INCLUDE_SIMGRID_ENGINE_H_

#include <simgrid/forward.h>
#include <stddef.h>

SG_BEGIN_DECL /* C interface */
/** Initialize the SimGrid engine, taking the command line parameters of your main function. */
XBT_PUBLIC void simgrid_init(int* argc, char** argv);

/** Creates a new platform, including hosts, links, and the routing table.
 *
 * \beginrst
 * See also: :ref:`platform`.
 * \endrst
 */
XBT_PUBLIC void simgrid_load_platform(const char* filename);
/** Load a deployment file and launch the actors that it contains
 *
 * \beginrst
 * See also: :ref:`deploy`.
 * \endrst
 */
XBT_PUBLIC void simgrid_load_deployment(const char* filename);
/** Run the simulation after initialization */
XBT_PUBLIC void simgrid_run();
/** Registers the main function of an actor that will be launched from the deployment file */
XBT_PUBLIC void simgrid_register_function(const char* name, void (*code)(int, char**));
/** Registers a function as the default main function of actors
 *
 * It will be used as fallback when the function requested from the deployment file was not registered.
 * It is used for trace-based simulations (see examples/s4u/replay-comms and similar).
 */
XBT_PUBLIC void simgrid_register_default(void (*code)(int, char**));
/** Retrieve the simulation time (in seconds) */
XBT_PUBLIC double simgrid_get_clock();
/** Retrieve the number of actors in the simulation */
XBT_PUBLIC int simgrid_get_actor_count();

/** @brief Allow other libraries to react to the --help flag, too
 *
 * When finding --help on the command line, simgrid usually stops right after displaying its help message.
 * If you are writing a library using simgrid, you may want to display your own help message before everything stops.
 * If so, just call this function before having simgrid parsing the command line, and you will be given the control
 * even if the user is asking for help.
 */
XBT_PUBLIC void sg_config_continue_after_help();

XBT_PUBLIC void simgrid_get_all_hosts(size_t* host_count, sg_host_t** hosts);

SG_END_DECL

#endif /* INCLUDE_SIMGRID_ENGINE_H_ */
