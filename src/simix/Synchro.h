/* Copyright (c) 2007-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SIMIX_SYNCHRO_HPP
#define _SIMIX_SYNCHRO_HPP

#include <xbt/base.h>
#include "simgrid/forward.h"

#ifdef __cplusplus

#include <simgrid/simix.hpp>

namespace simgrid {
namespace simix {

  XBT_PUBLIC_CLASS Synchro {
  public:
    Synchro();
    virtual ~Synchro();
    e_smx_state_t state;               /* State of the synchro */
    char *name = nullptr;              /* synchro name if any */
    xbt_fifo_t simcalls;               /* List of simcalls waiting for this synchro */
    char *category = nullptr;          /* For instrumentation */

    virtual void suspend()=0;
    virtual void resume()=0;
    virtual void post() =0; // What to do when a simcall terminates

    void ref();
    void unref();
  private:
    int refcount=1;
  };
}} // namespace simgrid::simix
#else /* not C++ */


#endif

#endif
