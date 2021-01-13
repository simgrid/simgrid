/* Copyright (c) 2008-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MC_MC_H
#define MC_MC_H

#include <simgrid/forward.h>
#include <simgrid/modelchecker.h> /* our public interface (and definition of SIMGRID_HAVE_MC) */

/* Maximum size of the application heap.
 *
 * The model-checker heap is placed at this offset from the beginning of the application heap.
 *
 * In the current implementation, if the application uses more than this for the application heap the application heap
 * will smash the beginning of the model-checker heap and bad things will happen.
 *
 * For 64 bits systems, we have a lot of virtual memory available so we wan use a much bigger value in order to avoid
 * bad things from happening.
 * */

#define STD_HEAP_SIZE   (sizeof(void*)<=4 ? (100*1024*1024) : (1ll*1024*1024*1024*1024))

SG_BEGIN_DECL

/********************************* Global *************************************/
XBT_ATTRIB_NORETURN XBT_PUBLIC void MC_run();
XBT_PRIVATE void MC_automaton_load(const char *file);

SG_END_DECL

#ifdef __cplusplus
XBT_PUBLIC void MC_process_clock_add(const simgrid::kernel::actor::ActorImpl*, double);
XBT_PUBLIC double MC_process_clock_get(const simgrid::kernel::actor::ActorImpl*);
#endif

#endif
